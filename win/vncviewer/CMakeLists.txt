include_directories(${CMAKE_BINARY_DIR}/win)

# Disable auto-generated manifests, since we have our own
if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif()

add_executable(vncviewer WIN32
  buildTime.cxx
  CConn.cxx
  CConnOptions.cxx
  CConnThread.cxx
  ConnectingDialog.cxx
  ConnectionDialog.cxx
  DesktopWindow.cxx
  InfoDialog.cxx
  OptionsDialog.cxx
  UserPasswdDialog.cxx
  ViewerToolBar.cxx
  vncviewer.cxx
  vncviewer.rc)

target_link_libraries(vncviewer rfb rfb_win32 Xregion network rdr
  ws2_32.lib)

install(TARGETS vncviewer
  RUNTIME DESTINATION .
)
