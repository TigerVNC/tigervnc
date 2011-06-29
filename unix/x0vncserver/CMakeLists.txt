include_directories(${X11_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/unix/tx)
include_directories(${CMAKE_SOURCE_DIR}/unix)
include_directories(${CMAKE_SOURCE_DIR}/common)

add_executable(x0vncserver
  buildtime.c
  Geometry.cxx
  Image.cxx
  PollingManager.cxx
  PollingScheduler.cxx
  TimeMillis.cxx
  x0vncserver.cxx
  XPixelBuffer.cxx
  ../vncconfig/QueryConnectDialog.cxx
)

target_link_libraries(x0vncserver tx rfb network rdr ${X11_LIBRARIES})

install(TARGETS x0vncserver DESTINATION bin)
install(FILES x0vncserver.man DESTINATION man/man1 RENAME xvnc0server.1)
