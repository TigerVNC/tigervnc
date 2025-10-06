if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(w0vncserver
  portals/portalConstants.h
  portals/PortalProxy.cxx
  portals/RemoteDesktop.cxx
  w0vncserver.cxx
  GSocketSource.cxx
  pipewire/PipeWirePixelBuffer.cxx
  pipewire/PipeWireSource.cxx
  pipewire/PipeWireStream.cxx
  PortalDesktop.cxx
  RFBTimerSource.cxx
)

target_include_directories(w0vncserver SYSTEM PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GIO_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GOBJECT_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIXMAN_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIPEWIRE_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${UUID_INCLUDE_DIRS})

target_link_libraries(w0vncserver core rfb network rdr)
target_link_libraries(w0vncserver ${GLIB_LIBRARIES})
target_link_libraries(w0vncserver ${GIO_LIBRARIES})
target_link_libraries(w0vncserver ${GOBJECT_LIBRARIES})
target_link_libraries(w0vncserver ${PIXMAN_LIBRARIES})
target_link_libraries(w0vncserver ${PIPEWIRE_LIBRARIES})
target_link_libraries(w0vncserver ${UUID_LIBRARIES})

install(TARGETS w0vncserver DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES w0vncserver.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME w0vncserver.1)
