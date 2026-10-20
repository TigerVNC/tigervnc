if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(vncconfig 
  vncExt.c
  vncconfig.cxx
  QueryConnectDialog.cxx)

target_link_libraries(vncconfig PRIVATE core tx rfb)
target_include_directories(vncconfig SYSTEM PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(vncconfig PRIVATE ${X11_LIBRARIES})

install(TARGETS vncconfig DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES vncconfig.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME vncconfig.1)
