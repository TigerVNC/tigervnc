include_directories(${X11_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/unix/tx)

add_executable(vncconfig 
  buildtime.c
  vncExt.c
  vncconfig.cxx
  QueryConnectDialog.cxx)

target_link_libraries(vncconfig tx rfb network rdr ${X11_LIBRARIES})

install(TARGETS vncconfig DESTINATION ${BIN_DIR})
install(FILES vncconfig.man DESTINATION ${MAN_DIR}/man1 RENAME vncconfig.1)
