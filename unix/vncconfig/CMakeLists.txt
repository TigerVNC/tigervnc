project(vncconfig)

add_executable(${PROJECT_NAME} 
  buildtime.c
  vncExt.c
  vncconfig.cxx
  QueryConnectDialog.cxx)

target_link_libraries(${PROJECT_NAME} tx ${X11_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
    
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        COMPONENT Runtime)
install(FILES ${PROJECT_NAME}.man
    DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME ${PROJECT_NAME}.1
        COMPONENT Runtime
    )
