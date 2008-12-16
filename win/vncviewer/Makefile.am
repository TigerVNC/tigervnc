bin_PROGRAMS = vncviewer

HDRS = \
  CConn.h \
  CConnOptions.h \
  CConnThread.h \
  ConnectingDialog.h \
  ConnectionDialog.h \
  DesktopWindow.h \
  FileTransfer.h \
  FTBrowseDlg.h \
  FTDialog.h \
  FTListView.h \
  FTProgress.h \
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
  FileTransfer.cxx \
  FTBrowseDlg.cxx \
  FTDialog.cxx \
  FTListView.cxx \
  FTProgress.cxx \
  InfoDialog.cxx \
  OptionsDialog.cxx \
  UserPasswdDialog.cxx \
  ViewerToolBar.cxx \
  vncviewer.cxx

vncviewer_CPPFLAGS = -I$(COMMON_DIR) -I$(top_srcdir)

vncviewer_LDADD = $(COMMON_DIR)/rfb/librfb.la \
    $(top_srcdir)/rfb_win32/librfb_win32.la \
    $(COMMON_DIR)/Xregion/libXregion.la \
    $(COMMON_DIR)/network/libnetwork.la \
    $(COMMON_DIR)/rdr/librdr.la \
    resources.o -lws2_32 -lgdi32 -luser32 -lcomctl32 -lversion -lcomdlg32

vncviewer_LDFLAGS = -I$(top_srcdir)/win -mwindows

resources.o: vncviewer.rc
	i386-mingw32msvc-windres $^ -o $@
