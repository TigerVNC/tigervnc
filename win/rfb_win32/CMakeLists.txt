add_library(rfb_win32 STATIC
  AboutDialog.cxx
  Clipboard.cxx
  CurrentUser.cxx
  DeviceContext.cxx
  DeviceFrameBuffer.cxx
  Dialog.cxx
  DIBSectionBuffer.cxx
  EventManager.cxx
  LaunchProcess.cxx
  ListViewControl.cxx
  MonitorInfo.cxx
  MsgWindow.cxx
  RegConfig.cxx
  Registry.cxx
  SecurityPage.cxx
  SDisplayCorePolling.cxx
  SDisplayCoreWMHooks.cxx
  SDisplay.cxx
  Security.cxx
  Service.cxx
  SInput.cxx
  SocketManager.cxx
  TCharArray.cxx
  TsSessions.cxx
  Win32Util.cxx
  WMCursor.cxx
  WMHooks.cxx
  WMNotifier.cxx
  WMPoller.cxx
  WMShatter.cxx
  WMWindowCopyRect.cxx)

if(BUILD_WINVNC)
  target_sources(rfb_win32 PRIVATE ${RFB_WIN32_SOURCES} CleanDesktop.cxx)
endif()

target_link_libraries(rfb_win32 user32.lib comctl32.lib wtsapi32.lib version.lib)
