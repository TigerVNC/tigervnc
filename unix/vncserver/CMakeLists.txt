configure_file(vncserver@.service.in vncserver@.service)
configure_file(vncserver.sysv.in vncserver.sysv)

install(PROGRAMS vncserver DESTINATION ${BIN_DIR})
install(FILES vncserver.man DESTINATION ${MAN_DIR}/man1 RENAME vncserver.1)
install(FILES vncserver-config-defaults vncserver-config-mandatory DESTINATION ${SYSCONF_DIR}/tigervnc)

if(INSTALL_SYSTEMD_UNITS)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vncserver@.service DESTINATION ${UNIT_DIR})
endif()

if(INSTALL_SYSV_INIT)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncserver.sysv DESTINATION ${SYSCONF_DIR}/init.d)
endif()
