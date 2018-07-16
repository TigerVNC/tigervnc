add_executable(vncsession vncsession.c)
target_link_libraries(vncsession ${PAM_LIBS})

configure_file(vncserver@.service.in vncserver@.service @ONLY)
configure_file(vncsession-start.in vncsession-start @ONLY)
configure_file(vncserver.in vncserver @ONLY)

install(TARGETS vncsession DESTINATION ${SBIN_DIR})
install(FILES tigervnc.pam DESTINATION ${SYSCONF_DIR}/pam.d RENAME tigervnc)
install(FILES vncsession.man DESTINATION ${MAN_DIR}/man8 RENAME vncsession.8)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncserver DESTINATION ${LIBEXEC_DIR})
install(FILES vncserver.man DESTINATION ${MAN_DIR}/man8 RENAME vncserver.8)
install(FILES vncserver-config-defaults vncserver-config-mandatory DESTINATION ${SYSCONF_DIR}/tigervnc)

install(FILES vncserver.users DESTINATION ${SYSCONF_DIR}/tigervnc)

if(INSTALL_SYSTEMD_UNITS)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vncserver@.service DESTINATION ${UNIT_DIR})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncsession-start DESTINATION ${LIBEXEC_DIR})
endif()
