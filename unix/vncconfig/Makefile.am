bin_PROGRAMS = vncconfig

man1_MANS = vncconfig.man

HDRS = QueryConnectDialog.h vncExt.h

vncconfig_SOURCES = $(HDRS) vncExt.c vncconfig.cxx QueryConnectDialog.cxx \
	buildtime.c

# X_CFLAGS are really CPPFLAGS
vncconfig_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/unix/tx @X_CFLAGS@

vncconfig_LDADD = $(top_builddir)/unix/tx/libtx.la \
	$(top_builddir)/common/rfb/librfb.la \
	$(top_builddir)/common/network/libnetwork.la \
	$(top_builddir)/common/rdr/librdr.la @X_PRE_LIBS@ @X_LIBS@ -lX11 -lXext \
	@X_EXTRA_LIBS@

EXTRA_DIST = vncconfig.man
