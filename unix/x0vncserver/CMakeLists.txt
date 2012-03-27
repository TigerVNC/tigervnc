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
if(X11_FOUND AND X11_XTest_LIB)
  add_definitions(-DHAVE_XTEST)
  target_link_libraries(x0vncserver ${X11_XTest_LIB})
else()
  message(WARNING "No XTest extension.  x0vncserver will be view-only.")
endif()

install(TARGETS x0vncserver DESTINATION ${BIN_DIR})
install(FILES x0vncserver.man DESTINATION ${MAN_DIR}/man1 RENAME x0vncserver.1)
