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

set(VNCVIEWER_LIBRARIES rfb rfb_win32 Xregion network rdr)

# When building with GnuTLS, librdr depends on ws2_32, so in order to make
# MinGW happy, we need to put ws2_32 in librdr's target_link_libraries string,
# not here.
if(NOT GNUTLS_FOUND)
  set(VNCVIEWER_LIBRARIES ${VNCVIEWER_LIBRARIES} ws2_32)
endif()

target_link_libraries(vncviewer ${VNCVIEWER_LIBRARIES})

install(TARGETS vncviewer
  RUNTIME DESTINATION .
)
