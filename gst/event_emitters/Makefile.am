plugin_LTLIBRARIES = libgsteventemitters.la

libgstaudiovisualizers_la_SOURCES = plugin.c \
	gstpowereventemitter.c gstpowereventemitter.h

libgstaudiovisualizers_la_CFLAGS = $(GST_PLUGINS_BAD_CFLAGS) \
	$(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) \
	$(GST_CFLAGS)
libgstaudiovisualizers_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) -lgstaudio-$(GST_API_VERSION) \
	-lgstvideo-$(GST_API_VERSION) -lgstfft-$(GST_API_VERSION) \
	-lgstpbutils-$(GST_API_VERSION) $(GST_BASE_LIBS)  $(GST_LIBS) $(LIBM)
libgstaudiovisualizers_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = gstpowereventemitter.h \
