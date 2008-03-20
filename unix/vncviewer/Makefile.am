bin_PROGRAMS = vncviewer

vncviewer_SOURCES = AboutDialog.h buildtime.c CConn.cxx CConn.h \
	DesktopWindow.cxx DesktopWindow.h InfoDialog.h OptionsDialog.h \
	parameters.h PasswdDialog.h ServerDialog.h vncviewer.cxx \
	gettext.h

vncviewer_CPPFLAGS = -I$(COMMON_DIR) -I$(TX_DIR) \
	-I$(UNIX_DIR) -DLOCALEDIR=\"$(localedir)\"

vncviewer_LDADD = $(TX_LIBS) $(RFB_LIBS) $(NETWORK_LIBS) $(RDR_LIBS) -lXext \
	-lX11 $(JPEG_LIBS) $(ZLIB_LIBS) $(LIBINTL)

