if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(w0vncserver
  w0vncserver.cxx
  RFBTimerSource.cxx
  GSocketMonitor.cxx
  portals/Portal.cxx
  portals/RemoteDesktop.cxx
  PipewirePixelBuffer.cxx
  PortalDesktop.cxx
)

set(WAYLAND_LIBRARIES ${GLIB_LIBRARIES} ${GIO_LIBRARIES} ${PIPEWIRE_LIBRARIES} ${UUID_LIBRARIES})
set(WAYLAND_LIBRARY_DIRS ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} ${PIPEWIRE_INCLUDE_DIRS})

target_include_directories(w0vncserver SYSTEM PUBLIC ${WAYLAND_LIBRARY_DIRS})

target_link_libraries(w0vncserver core rfb network rdr unixcommon)
target_link_libraries(w0vncserver ${WAYLAND_LIBRARIES})
