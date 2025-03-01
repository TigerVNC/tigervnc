add_executable(vncconfig 
  buildtime.c
  vncExt.c
  vncconfig.cxx
  QueryConnectDialog.cxx)

target_include_directories(vncconfig SYSTEM PUBLIC ${X11_INCLUDE_DIR})

target_include_directories(vncconfig PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(vncconfig PUBLIC ${CMAKE_SOURCE_DIR}/unix/tx)

target_link_libraries(vncconfig tx tigervnc ${X11_LIBRARIES})

install(TARGETS vncconfig
  DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    COMPONENT Runtime)
install(FILES vncconfig.man
  DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1
    COMPONENT Runtime
  RENAME vncconfig.1
  )
