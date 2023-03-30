add_executable(winvnc4 WIN32
  buildTime.cxx
  ControlPanel.cxx
  ManagedListener.cxx
  QueryConnectDialog.cxx
  STrayIcon.cxx
  VNCServerService.cxx
  VNCServerWin32.cxx
  winvnc.cxx
  winvnc.rc)

target_include_directories(winvnc4 PUBLIC ${CMAKE_BINARY_DIR}/win)
target_include_directories(winvnc4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(winvnc4 rfb rfb_win32 network rdr ws2_32.lib)

install(TARGETS winvnc4
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
