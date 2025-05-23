if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(w0vncserver
  w0vncserver.cxx
  RFBTimerSource.cxx
  GSocketSource.cxx
  portals/Portal.cxx
  portals/RemoteDesktop.cxx
  PortalDesktop.cxx
  PipeWirePixelBuffer.cxx
  PipeWireSource.cxx
)

target_include_directories(w0vncserver SYSTEM PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GIO_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIPEWIRE_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${UUID_INCLUDE_DIRS})

target_link_libraries(w0vncserver core rfb network rdr unixcommon)
target_link_libraries(w0vncserver ${GLIB_LIBRARIES})
target_link_libraries(w0vncserver ${GIO_LIBRARIES})
target_link_libraries(w0vncserver ${PIPEWIRE_LIBRARIES})
target_link_libraries(w0vncserver ${UUID_LIBRARIES})
