project(vncconfig)

add_executable(${PROJECT_NAME} WIN32
  Legacy.cxx
  Legacy.h
  PasswordDialog.cxx
  PasswordDialog.h
  vncconfig.cxx
  vncconfig.rc)

target_link_libraries(${PROJECT_NAME} PRIVATE rfb_win32 rfb network)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/win)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    VERSION ${VERSION})

INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Runtime
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT Runtime
    )
