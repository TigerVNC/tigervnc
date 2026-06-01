if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(vncpasswd 
  vncpasswd.cxx)

target_include_directories(vncpasswd PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(vncpasswd core tx rfb)

if(PWQUALITY_FOUND)
  target_include_directories(vncpasswd SYSTEM PRIVATE ${PWQUALITY_INCLUDE_DIRS})
  target_link_libraries(vncpasswd ${PWQUALITY_LIBRARIES})
endif()

install(TARGETS vncpasswd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES vncpasswd.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME vncpasswd.1)
