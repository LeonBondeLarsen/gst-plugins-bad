/* GStreamer
 * Copyright (C) <2011> Stefan Kost <ensonic@users.sf.net>
 *
 * gstspectrogramscope.h: simple oscilloscope
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

#ifndef __GST_SPECTROGRAM_SCOPE_H__
#define __GST_SPECTROGRAM_SCOPE_H__

#include "gst/pbutils/gstaudiovisualizer.h"
#include <gst/fft/gstffts16.h>
#include <stdio.h>

#define SPECTROGRAM_WIDTH 320
#define SPECTROGRAM_HIGHT 200

G_BEGIN_DECLS
#define GST_TYPE_SPECTROGRAM_SCOPE            (gst_spectrogram_scope_get_type())
#define GST_SPECTROGRAM_SCOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPECTROGRAM_SCOPE,GstSpectrogramScope))
#define GST_SPECTROGRAM_SCOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SPECTROGRAM_SCOPE,GstSpectrogramScopeClass))
#define GST_IS_SPECTROGRAM_SCOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SPECTROGRAM_SCOPE))
#define GST_IS_SPECTROGRAM_SCOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SPECTROGRAM_SCOPE))
typedef struct _GstSpectrogramScope GstSpectrogramScope;
typedef struct _GstSpectrogramScopeClass GstSpectrogramScopeClass;

struct _GstSpectrogramScope
{
  GstAudioVisualizer parent;

  GstFFTS16 *fft_ctx;
  GstFFTS16Complex *freq_data;
};

struct _GstSpectrogramScopeClass
{
  GstAudioVisualizerClass parent_class;
};

GType gst_spectrogram_scope_get_type (void);
gboolean gst_spectrogram_scope_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_SPECTROGRAM_SCOPE_H__ */
