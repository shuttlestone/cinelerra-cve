plugin_LTLIBRARIES = threshold.la
threshold_la_LDFLAGS = -avoid-version -module -shared 
threshold_la_LIBADD =
threshold_la_SOURCES = threshold.C thresholdwindow.C histogramengine.C

AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = \
	histogramengine.h histogramengine.inc \
	threshold.h threshold.inc \
	thresholdwindow.h thresholdwindow.inc
EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
