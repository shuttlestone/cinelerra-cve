plugin_LTLIBRARIES = denoiseseltempavg.la
denoiseseltempavg_la_LDFLAGS = -avoid-version -module -shared 
denoiseseltempavg_la_LIBADD = 
denoiseseltempavg_la_SOURCES = seltempavg.C seltempavgwindow.C 
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = seltempavg.h seltempavgwindow.h 
EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
