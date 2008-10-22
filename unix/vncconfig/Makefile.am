bin_PROGRAMS = vncconfig

man1_MANS = vncconfig.man

HDRS = QueryConnectDialog.h vncExt.h

vncconfig_SOURCES = $(HDRS) vncExt.c vncconfig.cxx QueryConnectDialog.cxx \
	buildtime.c

vncconfig_CPPFLAGS = -I$(COMMON_DIR) -I$(top_srcdir)/tx @X_CFLAGS@ # X_CFLAGS are really CPPFLAGS

vncconfig_LDADD = $(top_srcdir)/tx/libtx.la $(COMMON_DIR)/rfb/librfb.la \
	$(COMMON_DIR)/network/libnetwork.la $(COMMON_DIR)/rdr/librdr.la \
	@X_PRE_LIBS@ @X_LIBS@ -lX11 -lXext @X_EXTRA_LIBS@

EXTRA_DIST = vncconfig.man
