bin_PROGRAMS = winvnc4

HDRS = AddNewClientDialog.h ControlPanel.h JavaViewer.h ManagedListener.h \
	QueryConnectDialog.h STrayIcon.h VNCServerService.h VNCServerWin32.h \
	resource.h

winvnc4_SOURCES = $(HDRS) buildTime.cxx ControlPanel.cxx JavaViewer.cxx \
	ManagedListener.cxx QueryConnectDialog.cxx STrayIcon.cxx \
	VNCServerService.cxx VNCServerWin32.cxx winvnc.cxx

winvnc4_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/win

winvnc4_LDADD = $(top_builddir)/common/rfb/librfb.la \
	$(top_builddir)/win/rfb_win32/librfb_win32.la \
	$(top_builddir)/common/Xregion/libXregion.la \
	$(top_builddir)/common/network/libnetwork.la \
	$(top_builddir)/common/rdr/librdr.la resources.o -lws2_32 -lgdi32 \
	-lole32 -lcomctl32 -lversion

winvnc4_LDFLAGS = -mwindows

EXTRA_DIST = winvnc.rc winvnc.ico connected.ico icon_dis.ico connecte.ico \
	winvnc4.exe.manifest winvnc.bmp

resources.o: winvnc.rc
	$(WINDRES) $^ -o $@

