set(RFB_WIN32_SOURCES
  AboutDialog.cxx
  CKeyboard.cxx
  Clipboard.cxx
  CPointer.cxx
  CurrentUser.cxx
  DeviceContext.cxx
  DeviceFrameBuffer.cxx
  Dialog.cxx
  DIBSectionBuffer.cxx
  DynamicFn.cxx
  EventManager.cxx
  LaunchProcess.cxx
  ListViewControl.cxx
  LowLevelKeyEvents.cxx
  MonitorInfo.cxx
  MsgWindow.cxx
  OSVersion.cxx
  ProgressControl.cxx
  RegConfig.cxx
  Registry.cxx
  ScaledDIBSectionBuffer.cxx
  SecurityPage.cxx
  SDisplayCorePolling.cxx
  SDisplayCoreWMHooks.cxx
  SDisplay.cxx
  Security.cxx
  Service.cxx
  SInput.cxx
  SocketManager.cxx
  TCharArray.cxx
  Threading.cxx
  ToolBar.cxx
  TsSessions.cxx
  Win32Util.cxx
  WMCursor.cxx
  WMHooks.cxx
  WMNotifier.cxx
  WMPoller.cxx
  WMShatter.cxx
  WMWindowCopyRect.cxx)

if(BUILD_WINVNC)
  set(RFB_WIN32_SOURCES ${RFB_WIN32_SOURCES} CleanDesktop.cxx)
endif()

add_library(rfb_win32 STATIC ${RFB_WIN32_SOURCES})

target_link_libraries(rfb_win32 user32.lib comctl32.lib version.lib)
