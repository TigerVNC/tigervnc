add_executable(w0vncserver
  w0vncserver.cxx
  RFBTimerSource.cxx
  WSocketMonitor.cxx
  WDesktop.cxx
  WPixelBuffer.cxx
  portals/Screencast.cxx
  portals/Portal.cxx
  portals/RemoteDesktop.cxx
  Pipewire.cxx
  PortalBackend.cxx
  ../../common/rfb/PixelBuffer.cxx
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

find_package(Pixman REQUIRED)
find_library(UUID_LIBRARIES uuid)
find_path(UUID_INCLUDE_DIRS uuid/uuid.h)

target_include_directories(w0vncserver SYSTEM PUBLIC ${PIPEWIRE_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GIO_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIXMAN_INCLUDE_DIRS})
target_include_directories(w0vncserver PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(w0vncserver PUBLIC ${CMAKE_SOURCE_DIR}/unix/common)
target_include_directories(w0vncserver PUBLIC ${CMAKE_SOURCE_DIR}/unix)

target_link_libraries(w0vncserver
  ${PIPEWIRE_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${GIO_LIBRARIES}
  ${PIXMAN_LIBRARIES}
  ${UUID_LIBRARIES}
  core
  rfb
  rdr
  unixcommon
)

add_definitions(${PIPEWIRE_CFLAGS_OTHER})
add_definitions(${GLIB_CFLAGS_OTHER})
add_definitions(${GIO_CFLAGS_OTHER})
add_definitions(${PIXMAN_CFLAGS_OTHER})