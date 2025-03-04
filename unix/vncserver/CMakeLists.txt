add_executable(vncsession vncsession.c)
target_link_libraries(vncsession ${PAM_LIBS} ${SELINUX_LIBS})

configure_file(vncserver@.service.in vncserver@.service @ONLY)
configure_file(vncsession-start.in vncsession-start @ONLY)
configure_file(vncserver.in vncserver @ONLY)
configure_file(vncsession.man.in vncsession.man @ONLY)

install(TARGETS vncsession DESTINATION ${CMAKE_INSTALL_FULL_SBINDIR} COMPONENT Runtime)
install(FILES tigervnc.pam DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/pam.d COMPONENT Runtime RENAME tigervnc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vncsession.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man8 COMPONENT Runtime RENAME vncsession.8)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncserver DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR} COMPONENT Runtime)
install(FILES vncserver.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man8 COMPONENT Runtime RENAME vncserver.8)
install(FILES vncserver-config-defaults vncserver-config-mandatory DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/tigervnc COMPONENT Runtime)

install(FILES vncserver.users DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/tigervnc COMPONENT Runtime)

install(FILES HOWTO.md DESTINATION ${CMAKE_INSTALL_FULL_DOCDIR} COMPONENT Runtime)

if(INSTALL_SYSTEMD_UNITS)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vncserver@.service DESTINATION ${CMAKE_INSTALL_FULL_UNITDIR} COMPONENT Runtime)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncsession-start DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR} COMPONENT Runtime)
endif()
