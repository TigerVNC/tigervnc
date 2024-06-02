add_executable(vncpasswd 
  vncpasswd.cxx)

target_include_directories(vncpasswd PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(vncpasswd tx rfb os)

if(PWQUALITY_FOUND)
  target_link_libraries(vncpasswd pwquality)
endif()

install(TARGETS vncpasswd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES vncpasswd.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME vncpasswd.1)
