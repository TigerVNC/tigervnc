noinst_LTLIBRARIES = librfb_win32.la

HDRS = \
  AboutDialog.h \
  BitmapInfo.h \
  CKeyboard.h \
  CleanDesktop.h \
  Clipboard.h \
  CompatibleBitmap.h \
  ComputerName.h \
  CPointer.h \
  CurrentUser.h \
  DeviceContext.h \
  DeviceFrameBuffer.h \
  Dialog.h \
  DIBSectionBuffer.h \
  DynamicFn.h \
  EventManager.h \
  FolderManager.h \
  Handle.h \
  IconInfo.h \
  IntervalTimer.h \
  keymap.h \
  LaunchProcess.h \
  ListViewControl.h \
  LocalMem.h \
  LogicalPalette.h \
  LowLevelKeyEvents.h \
  ModuleFileName.h \
  MonitorInfo.h \
  MsgBox.h \
  MsgWindow.h \
  OSVersion.h \
  ProgressControl.h \
  RegConfig.h \
  Registry.h \
  ScaledDIBSectionBuffer.h \
  SDisplayCoreDriver.h \
  SDisplayCorePolling.h \
  SDisplayCoreWMHooks.h \
  SDisplay.h \
  Security.h \
  Service.h \
  SFileTransferManagerWin32.h \
  SFileTransferWin32.h \
  SInput.h \
  SocketManager.h \
  TCharArray.h \
  Threading.h \
  ToolBar.h \
  TrayIcon.h \
  TsSessions.h \
  Win32Util.h \
  WMCursor.h \
  WMHooks.h \
  WMNotifier.h \
  WMPoller.h \
  WMShatter.h \
  WMWindowCopyRect.h


##  CleanDesktop.cxx not built - MinGW does not provide COM

librfb_win32_la_SOURCES = $(HDRS) \
  AboutDialog.cxx \
  CKeyboard.cxx \
  Clipboard.cxx \
  CPointer.cxx \
  CurrentUser.cxx \
  DeviceContext.cxx \
  DeviceFrameBuffer.cxx \
  Dialog.cxx \
  DIBSectionBuffer.cxx \
  DynamicFn.cxx \
  EventManager.cxx \
  FolderManager.cxx \
  LaunchProcess.cxx \
  ListViewControl.cxx \
  LowLevelKeyEvents.cxx \
  MonitorInfo.cxx \
  MsgWindow.cxx \
  OSVersion.cxx \
  ProgressControl.cxx \
  RegConfig.cxx \
  Registry.cxx \
  ScaledDIBSectionBuffer.cxx \
  SDisplayCorePolling.cxx \
  SDisplayCoreWMHooks.cxx \
  SDisplay.cxx \
  Security.cxx \
  Service.cxx \
  SFileTransferManagerWin32.cxx \
  SFileTransferWin32.cxx \
  SInput.cxx \
  SocketManager.cxx \
  TCharArray.cxx \
  Threading.cxx \
  ToolBar.cxx \
  TsSessions.cxx \
  Win32Util.cxx \
  WMCursor.cxx \
  WMHooks.cxx \
  WMNotifier.cxx \
  WMPoller.cxx \
  WMShatter.cxx \
  WMWindowCopyRect.cxx

librfb_win32_la_CPPFLAGS = -I$(top_srcdir) -I$(COMMON_DIR)
librfb_win32_la_LIBADD =
