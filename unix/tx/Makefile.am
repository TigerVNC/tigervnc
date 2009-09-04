noinst_LTLIBRARIES = libtx.la

HDRS = TXButton.h TXCheckbox.h TXDialog.h TXEntry.h TXImage.h TXLabel.h \
	TXMenu.h TXMsgBox.h TXScrollbar.h TXViewport.h TXWindow.h

libtx_la_SOURCES = $(HDRS) TXWindow.cxx TXScrollbar.cxx TXViewport.cxx \
	TXImage.cxx TXMenu.cxx

libtx_la_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/common/rfb \
	@X_CFLAGS@ # X_CFLAGS are really CPPFLAGS
