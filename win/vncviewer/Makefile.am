bin_PROGRAMS = vncviewer

HDRS = \
  CConn.h \
  CConnOptions.h \
  CConnThread.h \
  ConnectingDialog.h \
  ConnectionDialog.h \
  DesktopWindow.h \
  InfoDialog.h \
  ListenServer.h \
  ListenTrayIcon.h \
  MRU.h \
  OptionsDialog.h \
  resource.h \
  UserPasswdDialog.h \
  ViewerToolBar.h 

vncviewer_SOURCES = $(HDRS) \
  buildTime.cxx \
  CConn.cxx \
  CConnOptions.cxx \
  CConnThread.cxx \
  ConnectingDialog.cxx \
  ConnectionDialog.cxx \
  DesktopWindow.cxx \
  InfoDialog.cxx \
  OptionsDialog.cxx \
  UserPasswdDialog.cxx \
  ViewerToolBar.cxx \
  vncviewer.cxx

vncviewer_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/win

vncviewer_LDADD = $(top_srcdir)/common/rfb/librfb.la \
    $(top_srcdir)/win/rfb_win32/librfb_win32.la \
    $(top_srcdir)/common/Xregion/libXregion.la \
    $(top_srcdir)/common/network/libnetwork.la \
    $(top_srcdir)/common/rdr/librdr.la \
    resources.o -lws2_32 -lgdi32 -luser32 -lcomctl32 -lversion -lcomdlg32

vncviewer_LDFLAGS = -I$(top_srcdir)/win -mwindows

EXTRA_DIST = vncviewer.rc vncviewer.ico cursor1.cur vncviewer.exe.manifest \
	vncviewer.bmp toolbar.bmp

resources.o: vncviewer.rc
	$(WINDRES) $^ -o $@
