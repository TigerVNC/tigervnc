if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(winvnc4 WIN32
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
target_link_libraries(winvnc4 rfbserver rfb rfb_win32 network rdr ws2_32.lib)

install(TARGETS winvnc4
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
