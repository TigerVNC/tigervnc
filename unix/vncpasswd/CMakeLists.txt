include_directories(${CMAKE_SOURCE_DIR}/common)

add_executable(vncpasswd 
  vncpasswd.cxx)

target_link_libraries(vncpasswd tx rfb os)

install(TARGETS vncpasswd DESTINATION ${BIN_INSTALL_DIR})
install(FILES vncpasswd.man DESTINATION ${MAN_INSTALL_DIR}/man1 RENAME vncpasswd.1)
