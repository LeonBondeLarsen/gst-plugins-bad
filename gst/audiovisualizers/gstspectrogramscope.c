/* GStreamer
 * Copyright (C) <2011> Stefan Kost <ensonic@users.sf.net>
 *
 * gstspectrogramscope.c: frequency spectrum scope
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.

 */
/**
 * SECTION:element-spectrogramscope
 * @title: spectrogramscope
 * @see_also: goom
 *
 * Spectrogramscope is a simple spectrum visualisation element. It renders the
 * frequency spectrum as a series of bars.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc ! audioconvert ! spectrogramscope ! ximagesink
 * ]|
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>

#include "gstspectrogramscope.h"

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define RGB_ORDER "xRGB"
#else
#define RGB_ORDER "BGRx"
#endif

// Enumeration of the properties
enum
{
  PROP_0,
  PROP_COLORMAP
};

// Enumeration of the options for the colormap property
enum
{
  COLORMAP_GREY = 0,
  COLORMAP_GOLD
};

// Function prototypes
static void gst_spectrogram_scope_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_spectrogram_scope_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_spectrogram_scope_finalize (GObject * object);
static gboolean gst_spectrogram_scope_setup (GstAudioVisualizer * scope);
static gboolean gst_spectrogram_scope_render (GstAudioVisualizer * scope,
    GstBuffer * audio, GstVideoFrame * video);
static gint32 gst_spectrogram_scope_greymap (gint);
static gint32 gst_spectrogram_scope_colormap (gint);

// Gstreamer declarations
#define GST_TYPE_SPECTROGRAM_SCOPE_STYLE (gst_spectrogram_scope_colormap_get_type ())
#define GST_CAT_DEFAULT spectrogram_scope_debug

GST_DEBUG_CATEGORY_STATIC (spectrogram_scope_debug);
G_DEFINE_TYPE (GstSpectrogramScope, gst_spectrogram_scope,
    GST_TYPE_AUDIO_VISUALIZER);

static GstStaticPadTemplate gst_spectrogram_scope_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (RGB_ORDER))
    );

static GstStaticPadTemplate gst_spectrogram_scope_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 8000, 96000 ], "
        "channels = (int) 2, " "channel-mask = (bitmask) 0x3")
    );

static GType
gst_spectrogram_scope_colormap_get_type (void)
{
  static GType gtype = 0;

  if (gtype == 0) {
    static const GEnumValue values[] = {
      {COLORMAP_GREY, "black to white (default)", "grey"},
      {COLORMAP_GOLD, "black to yellow to white", "gold"},
      {0, NULL, NULL}
    };

    gtype = g_enum_register_static ("GstSpectrogramScopeColormap", values);
  }
  return gtype;
}

static void
gst_spectrogram_scope_class_init (GstSpectrogramScopeClass * g_class)
{
  GObjectClass *gobject_class = (GObjectClass *) g_class;
  GstElementClass *element_class = (GstElementClass *) g_class;
  GstAudioVisualizerClass *scope_class = (GstAudioVisualizerClass *) g_class;

  gobject_class->finalize = gst_spectrogram_scope_finalize;

  gst_element_class_set_static_metadata (element_class,
      "Spectrogram scope", "Visualization",
      "Simple spectrogram scope", "Leon Bonde Larsen <leon@bondelarsen.dk>");

  gst_element_class_add_static_pad_template (element_class,
      &gst_spectrogram_scope_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_spectrogram_scope_sink_template);

  gobject_class->set_property = gst_spectrogram_scope_set_property;
  gobject_class->get_property = gst_spectrogram_scope_get_property;

  scope_class->setup = GST_DEBUG_FUNCPTR (gst_spectrogram_scope_setup);
  scope_class->render = GST_DEBUG_FUNCPTR (gst_spectrogram_scope_render);

  g_object_class_install_property (gobject_class, PROP_COLORMAP,
      g_param_spec_enum ("colormap", "colormap",
          "Colormap for the spectrogram scope display.",
          GST_TYPE_SPECTROGRAM_SCOPE_STYLE, COLORMAP_GREY,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
gst_spectrogram_scope_init (GstSpectrogramScope * scope)
{
}

static void
gst_spectrogram_scope_finalize (GObject * object)
{
  GstSpectrogramScope *scope = GST_SPECTROGRAM_SCOPE (object);

  if (scope->fft_ctx) {
    gst_fft_s16_free (scope->fft_ctx);
    scope->fft_ctx = NULL;
  }
  if (scope->freq_data) {
    g_free (scope->freq_data);
    scope->freq_data = NULL;
  }

  G_OBJECT_CLASS (gst_spectrogram_scope_parent_class)->finalize (object);
}

static gboolean
gst_spectrogram_scope_setup (GstAudioVisualizer * bscope)
{
  GstSpectrogramScope *scope = GST_SPECTROGRAM_SCOPE (bscope);
  guint video_width, video_height;

  video_width = GST_VIDEO_INFO_WIDTH (&bscope->vinfo);
  video_height = GST_VIDEO_INFO_HEIGHT (&bscope->vinfo);

  //guint num_freq = GST_VIDEO_INFO_HEIGHT (&bscope->vinfo) + 1;

  // Calculate number of samples needed per render() call
  bscope->req_spf = video_height * 2 - 2;

  // Allocate GstFFTS16 and GstFFTS16Complex objects
  if (scope->fft_ctx)
    gst_fft_s16_free (scope->fft_ctx);
  g_free (scope->freq_data);

  scope->fft_ctx = gst_fft_s16_new (bscope->req_spf, FALSE);
  scope->freq_data = g_new (GstFFTS16Complex, video_width + 1);

  // Allocate memory for holding the fft data for the spectrogram
  scope->fft_array =
      gst_allocator_alloc (NULL, video_width * video_height, NULL);
  gst_memory_map (scope->fft_array, &scope->fft_array_info,
      GST_MAP_READ | GST_MAP_WRITE);

  return TRUE;
}

static gint32
gst_spectrogram_scope_greymap (gint intensity)
{
  // Set all three colors to the current intensity
  return (intensity << 16) | (intensity << 8) | intensity;
}

static gint32
gst_spectrogram_scope_colormap (gint color)
{
  guint red, green, blue;
  static gfloat normalised_input, color_number, group, residue;

  // Update max value so far
  static gint max = 1;
  if (color > max) {
    max = color;
    printf ("New max = %d\n", max);
  }
  // Normalise input and find the color number, color group and the residue
  normalised_input = (gfloat) color / (gfloat) max;
  color_number = normalised_input / 0.25;
  group = floor (color_number);
  residue = floor (255 * (color_number - group));

  // Find color mix from group and residue
  switch ((gint) group) {
    case 0:                    //black to yellow
      red = residue;
      green = residue;
      blue = 0;
      break;
    case 1:                    //yellow to red
      red = 255;
      green = 255 - residue;
      blue = 0;
      break;
    case 2:                    //red to magenta
      red = 255;
      green = 0;
      blue = residue;
      break;
    case 3:                    //magenta to white
      red = 255;
      green = residue;
      blue = 255;
      break;
    default:
      red = 255;
      green = 255;
      blue = 255;
      break;
  }

  // Return 32-bit color
  return (red << 16) | (green << 8) | blue;
}

static gboolean
gst_spectrogram_scope_render (GstAudioVisualizer * bscope, GstBuffer * audio,
    GstVideoFrame * video)
{
  GstSpectrogramScope *scope = GST_SPECTROGRAM_SCOPE (bscope);

  gint16 *mono_adata;
  GstFFTS16Complex *fdata = scope->freq_data;

  guint x = 0, y = 0, x_ptr = 0, index = 0;
  guint video_width = GST_VIDEO_INFO_WIDTH (&bscope->vinfo);
  guint video_height = GST_VIDEO_INFO_HEIGHT (&bscope->vinfo) - 1;
  gfloat fr, fi;
  GstMapInfo amap;
  guint32 *vdata;
  guint32 off = 0;
  gint channels;
  static guint32 collumn_pointer = 0;

  // Map the video data
  vdata = (guint32 *) GST_VIDEO_FRAME_PLANE_DATA (video, 0);

  // Map the audio data
  gst_buffer_map (audio, &amap, GST_MAP_READ);
  mono_adata = (gint16 *) g_memdup (amap.data, amap.size);

  // If multiple channels, deinterleave and mixdown adata
  channels = GST_AUDIO_INFO_CHANNELS (&bscope->ainfo);
  if (channels > 1) {
    guint ch = channels;
    guint num_samples = amap.size / (ch * sizeof (gint16));
    guint i, c, v, s = 0;

    for (i = 0; i < num_samples; i++) {
      v = 0;
      for (c = 0; c < ch; c++) {
        v += mono_adata[s++];
      }
      mono_adata[i] = v / ch;
    }
  }
  // Run fft
  gst_fft_s16_window (scope->fft_ctx, mono_adata, GST_FFT_WINDOW_HAMMING);
  gst_fft_s16_fft (scope->fft_ctx, mono_adata, fdata);
  g_free (mono_adata);

  // Increment the pointer to the most resent collumn in the spectrogram
  collumn_pointer++;
  if (collumn_pointer == video_width) {
    collumn_pointer = 0;
  }
  // For each bin in the fft calculate intensity and push to fft_array
  for (y = 0; y < video_height; y++) {
    fr = (gfloat) fdata[1 + y].r / 512.0;
    fi = (gfloat) fdata[1 + y].i / 512.0;
    index = (y * video_width) + collumn_pointer;
    scope->fft_array_info.data[index] =
        (guint) (video_height * sqrt (fr * fr + fi * fi));
  }

  // For each bin in the spectrogram update the corresponding pixel in vdata
  for (x = 0; x < video_width; x++) {
    x_ptr = (collumn_pointer + x + 1) % video_width;
    for (y = 0; y < video_height; y++) {
      off = ((video_height - y - 1) * video_width) + x;
      index = (y * video_width) + x_ptr;
      vdata[off] =
          (*scope->colormap_function) (scope->fft_array_info.data[index]);
    }
  }

  // Unmap the audio data
  gst_buffer_unmap (audio, &amap);

  return TRUE;
}

static void
gst_spectrogram_scope_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSpectrogramScope *scope = GST_SPECTROGRAM_SCOPE (object);

  switch (prop_id) {
    case PROP_COLORMAP:
      scope->colormap = g_value_get_enum (value);
      switch (scope->colormap) {
        case COLORMAP_GREY:
          scope->colormap_function = (void *) gst_spectrogram_scope_greymap;
          break;
        case COLORMAP_GOLD:
          scope->colormap_function = (void *) gst_spectrogram_scope_colormap;
          break;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_spectrogram_scope_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSpectrogramScope *scope = GST_SPECTROGRAM_SCOPE (object);

  switch (prop_id) {
    case PROP_COLORMAP:
      g_value_set_enum (value, scope->colormap);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


gboolean
gst_spectrogram_scope_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (spectrogram_scope_debug, "spectrogramscope", 0,
      "spectrogramscope");

  return gst_element_register (plugin, "spectrogramscope", GST_RANK_NONE,
      GST_TYPE_SPECTROGRAM_SCOPE);
}
