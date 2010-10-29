bin_PROGRAMS = vncviewer

man1_MANS = vncviewer.man

HDRS = AboutDialog.h CConn.h DesktopWindow.h gettext.h InfoDialog.h \
	OptionsDialog.h parameters.h PasswdDialog.h ServerDialog.h

vncviewer_SOURCES = $(HDRS) DesktopWindow.cxx CConn.cxx vncviewer.cxx \
	buildtime.c
# X_CFLAGS are really CPPFLAGS
vncviewer_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/unix \
	-I$(top_srcdir)/unix/tx -I$(top_srcdir)/intl \
	-DLOCALEDIR=\"$(localedir)\" @X_CFLAGS@

vncviewer_LDADD = $(top_builddir)/unix/tx/libtx.la \
	$(top_builddir)/common/rfb/librfb.la \
	$(top_builddir)/common/network/libnetwork.la \
	$(top_builddir)/common/rdr/librdr.la \
	$(top_builddir)/common/os/libos.la \
	@X_PRE_LIBS@ @X_LIBS@ -lXext -lX11 @X_EXTRA_LIBS@ @GNUTLS_LIBS@ # @LIBINTL@

if INCLUDED_ZLIB
vncviewer_LDADD += $(top_builddir)/common/zlib/libz.la
endif

EXTRA_DIST = vncviewer.man
