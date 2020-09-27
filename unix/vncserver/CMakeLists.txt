project(vncsession)
add_executable(${PROJECT_NAME} vncsession.c)
target_link_libraries(${PROJECT_NAME} ${PAM_LIBS})

configure_file(vncserver@.service.in vncserver@.service @ONLY)
configure_file(vncsession-start.in vncsession-start @ONLY)
configure_file(vncserver.in vncserver @ONLY)

install(TARGETS ${PROJECT_NAME} 
    DESTINATION ${CMAKE_INSTALL_FULL_SBINDIR}
        COMPONENT Runtime)
install(FILES tigervnc.pam
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/pam.d RENAME tigervnc
        COMPONENT Runtime)
install(FILES vncsession.man
    DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man8 RENAME vncsession.8
        COMPONENT Runtime)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncserver
    DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR}
        COMPONENT Runtime)
install(FILES vncserver.man
    DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man8 RENAME vncserver.8
        COMPONENT Runtime)
install(FILES vncserver-config-defaults vncserver-config-mandatory
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/tigervnc
        COMPONENT Runtime)

install(FILES vncserver.users
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/tigervnc
        COMPONENT Runtime)

if(INSTALL_SYSTEMD_UNITS)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vncserver@.service
      DESTINATION ${CMAKE_INSTALL_FULL_UNITDIR}
        COMPONENT Runtime)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vncsession-start
      DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR}
        COMPONENT Runtime)
endif()
