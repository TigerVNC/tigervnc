include_directories(${X11_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/unix/tx)

add_executable(vncconfig 
  buildtime.c
  vncExt.c
  vncconfig.cxx
  QueryConnectDialog.cxx)

target_link_libraries(vncconfig tx rfb network rdr ${X11_LIBRARIES})

install(TARGETS vncconfig DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES vncconfig.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME vncconfig.1)
