install(PROGRAMS vncserver DESTINATION ${BIN_DIR})
install(FILES vncserver.man DESTINATION ${MAN_DIR}/man1 RENAME vncserver.1)
install(FILES vncserver-config-defaults vncserver-config-mandatory DESTINATION ${SYSCONF_DIR}/tigervnc)
