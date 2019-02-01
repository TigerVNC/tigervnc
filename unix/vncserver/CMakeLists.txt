add_executable(vncsession vncsession.c)
target_link_libraries(vncsession ${PAM_LIBS})

configure_file(vncserver@.service.in vncserver@.service @ONLY)
configure_file(vncsession-start.in vncsession-start @ONLY)
configure_file(vncserver.sysv.in vncserver.sysv @ONLY)

install(TARGETS vncsession DESTINATION ${SBIN_INSTALL_DIR})
install(FILES vncsession.man DESTINATION ${MAN_INSTALL_DIR}/man8 RENAME vncsession.8)
install(PROGRAMS vncserver DESTINATION ${LIBEXEC_INSTALL_DIR})
install(FILES vncserver.man DESTINATION ${MAN_INSTALL_DIR}/man8 RENAME vncserver.8)
install(FILES vncserver-config-defaults vncserver-config-mandatory DESTINATION ${SYSCONF_INSTALL_DIR}/tigervnc)

install(FILES vncserver.users DESTINATION ${SYSCONF_INSTALL_DIR}/tigervnc)

if(INSTALL_SYSTEMD_UNITS)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vncserver@.service DESTINATION ${UNIT_INSTALL_DIR})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncsession-start DESTINATION ${LIBEXEC_INSTALL_DIR})
endif()

if(INSTALL_SYSV_INIT)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncserver.sysv DESTINATION ${SYSCONF_INSTALL_DIR}/init.d RENAME vncserver)
endif()
