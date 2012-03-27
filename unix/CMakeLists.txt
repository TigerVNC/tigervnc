add_subdirectory(tx)

add_subdirectory(vncconfig)
add_subdirectory(vncpasswd)
add_subdirectory(x0vncserver)

install(PROGRAMS vncserver DESTINATION ${BIN_DIR})
install(FILES vncserver.man DESTINATION ${MAN_DIR}/man1 RENAME vncserver.1)
