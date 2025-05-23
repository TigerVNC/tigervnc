if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(w0vncserver
  w0vncserver.cxx
  RFBTimerSource.cxx
  WSocketMonitor.cxx
  portals/Portal.cxx
  portals/RemoteDesktop.cxx
  PipewirePixelBuffer.cxx
  PortalDesktop.cxx
  GSignalWrapper.cxx
)

target_include_directories(w0vncserver SYSTEM PUBLIC ${WAYLAND_LIBRARY_DIRS})

target_link_libraries(w0vncserver rfb)
target_link_libraries(w0vncserver ${WAYLAND_LIBRARIES})
