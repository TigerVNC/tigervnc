include_directories(${CMAKE_BINARY_DIR}/win ${CMAKE_CURRENT_SOURCE_DIR})

# Disable auto-generated manifests, since we have our own
if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif()

add_library(wm_hooks SHARED
  ../wm_hooks/wm_hooks.cxx
  ../wm_hooks/wm_hooks.def
  ../wm_hooks/wm_hooks.rc)

set(VNCVIEWER_JAR_PATH ${CMAKE_BINARY_DIR}/java/VncViewer.jar)
set(INDEX_VNC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/index.vnc)

configure_file(winvnc.rc.in winvnc.rc)

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
  ${CMAKE_CURRENT_BINARY_DIR}/winvnc.rc)

target_link_libraries(winvnc4 rfb rfb_win32 Xregion network rdr ws2_32.lib)

if(BUILD_JAVA)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/winvnc.rc
    PROPERTIES OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/java/VncViewer.jar)
endif()

install(TARGETS winvnc4
  RUNTIME DESTINATION .
)

install(TARGETS wm_hooks
  RUNTIME DESTINATION .
)
