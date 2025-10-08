if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(w0vncserver
  portals/portalConstants.h
  portals/PortalProxy.cxx
  portals/RemoteDesktop.cxx
  portals/PortalDesktop.cxx
  pipewire/PipeWirePixelBuffer.cxx
  pipewire/PipeWireSource.cxx
  pipewire/PipeWireStream.cxx
  wayland/GWaylandSource.cxx
  wayland/objects/Output.cxx
  wayland/objects/Object.cxx
  wayland/objects/Display.cxx
  wayland/objects/Keyboard.cxx
  wayland/objects/Seat.cxx
  wayland/objects/Shm.cxx
  wayland/objects/ShmPool.cxx
  wayland/objects/VirtualPointer.cxx
  wayland/objects/VirtualKeyboard.cxx
  wayland/objects/ScreencopyManager.cxx
  wayland/WaylandDesktop.cxx
  wayland/WaylandPixelBuffer.cxx
  w0vncserver.cxx
  parameters.cxx
  GSocketSource.cxx
  RFBTimerSource.cxx
)

# Generate protocol headers
set(GEN_WAYLAND_SRCS "")
set(WAYLAND_PROTOCOLS
  wlr-screencopy-unstable-v1
  wlr-virtual-pointer-unstable-v1
  virtual-keyboard-unstable-v1
)

foreach(P ${WAYLAND_PROTOCOLS})
  set(XML ${CMAKE_CURRENT_SOURCE_DIR}/wayland/protocols/${P}.xml)
  set(H ${CMAKE_CURRENT_BINARY_DIR}/${P}.h)
  set(C ${CMAKE_CURRENT_BINARY_DIR}/${P}.c)

  add_custom_command(
    OUTPUT ${C}
    BYPRODUCTS ${H}
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${XML} ${C}
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${XML} ${H}
    DEPENDS ${XML}
    COMMENT "Generating ${C} and ${H}"
  )

  set_source_files_properties(${H} ${C} PROPERTIES GENERATED TRUE)

  list(APPEND GEN_WAYLAND_SRCS ${C})
endforeach()

# Add generated sources and headers
target_sources(w0vncserver PRIVATE ${GEN_WAYLAND_SRCS})
target_include_directories(w0vncserver PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(w0vncserver SYSTEM PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GIO_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GOBJECT_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIXMAN_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIPEWIRE_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${UUID_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${WAYLAND_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${XKBCOMMON_INCLUDE_DIRS})

target_link_libraries(w0vncserver core rfb network rdr)
target_link_libraries(w0vncserver ${GLIB_LIBRARIES})
target_link_libraries(w0vncserver ${GIO_LIBRARIES})
target_link_libraries(w0vncserver ${GOBJECT_LIBRARIES})
target_link_libraries(w0vncserver ${PIXMAN_LIBRARIES})
target_link_libraries(w0vncserver ${PIPEWIRE_LIBRARIES})
target_link_libraries(w0vncserver ${UUID_LIBRARIES})
target_link_libraries(w0vncserver ${XKBCOMMON_LIBRARIES})
target_link_libraries(w0vncserver ${WAYLAND_LIBRARIES})

install(TARGETS w0vncserver DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES w0vncserver.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME w0vncserver.1)
