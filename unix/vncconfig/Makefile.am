bin_PROGRAMS = vncconfig

man1_MANS = vncconfig.man

HDRS = QueryConnectDialog.h vncExt.h

vncconfig_SOURCES = $(HDRS) vncExt.c vncconfig.cxx QueryConnectDialog.cxx \
	buildtime.c

# X_CFLAGS are really CPPFLAGS
vncconfig_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/unix/tx @X_CFLAGS@

vncconfig_LDADD = $(top_srcdir)/unix/tx/libtx.la \
	$(top_srcdir)/common/rfb/librfb.la \
	$(top_srcdir)/common/network/libnetwork.la \
	$(top_srcdir)/common/rdr/librdr.la @X_PRE_LIBS@ @X_LIBS@ -lX11 -lXext \
	@X_EXTRA_LIBS@

EXTRA_DIST = vncconfig.man
