bin_PROGRAMS = vncviewer

man1_MANS = vncviewer.man

HDRS = AboutDialog.h CConn.h DesktopWindow.h gettext.h InfoDialog.h \
	OptionsDialog.h parameters.h PasswdDialog.h ServerDialog.h

vncviewer_SOURCES = $(HDRS) DesktopWindow.cxx CConn.cxx vncviewer.cxx \
	buildtime.c

vncviewer_CPPFLAGS = -I$(COMMON_DIR) -I$(top_srcdir) -I$(top_srcdir)/tx \
	-I$(top_srcdir)/intl -DLOCALEDIR=\"$(localedir)\" @X_CFLAGS@ # X_CFLAGS are really CPPFLAGS

vncviewer_LDADD = $(top_srcdir)/tx/libtx.la $(COMMON_DIR)/rfb/librfb.la \
	$(COMMON_DIR)/network/libnetwork.la $(COMMON_DIR)/rdr/librdr.la \
	@X_PRE_LIBS@ @X_LIBS@ -lXext -lX11 @X_EXTRA_LIBS@ # @LIBINTL@

EXTRA_DIST = vncviewer.man
