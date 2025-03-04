add_executable(vncpasswd 
  vncpasswd.cxx)

target_include_directories(vncpasswd PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(vncpasswd tx tigervnc)

if(PWQUALITY_FOUND)
  target_link_libraries(vncpasswd pwquality)
endif()

install(TARGETS vncpasswd
  DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
  COMPONENT Runtime)
install(FILES vncpasswd.man
  DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1
  COMPONENT Runtime
  RENAME vncpasswd.1
  )
