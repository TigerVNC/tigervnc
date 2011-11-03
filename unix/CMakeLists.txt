add_subdirectory(tx)

add_subdirectory(vncconfig)
add_subdirectory(vncpasswd)
add_subdirectory(x0vncserver)

install(PROGRAMS vncserver DESTINATION bin)
install(FILES vncserver.man DESTINATION man/man1 RENAME vncserver.1)
