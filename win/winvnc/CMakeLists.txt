include_directories(${CMAKE_BINARY_DIR}/win)

# Disable auto-generated manifests, since we have our own
if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif()

add_library(wm_hooks SHARED
  ../wm_hooks/wm_hooks.cxx
	../wm_hooks/wm_hooks.def
	../wm_hooks/wm_hooks.rc)

add_executable(winvnc4 WIN32
  buildTime.cxx
  ControlPanel.cxx
  JavaViewer.cxx
  ManagedListener.cxx
  QueryConnectDialog.cxx
  STrayIcon.cxx
  VNCServerService.cxx
  VNCServerWin32.cxx
  winvnc.cxx
  winvnc.rc)

target_link_libraries(winvnc4 rfb rfb_win32 Xregion network rdr ws2_32.lib)

install(TARGETS winvnc4
  RUNTIME DESTINATION .
)

install(TARGETS wm_hooks
  RUNTIME DESTINATION .
)
