if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(vncconfig WIN32
  Legacy.cxx
  PasswordDialog.cxx
  vncconfig.cxx
  vncconfig.rc)

target_include_directories(vncconfig PUBLIC ${CMAKE_BINARY_DIR}/win)
target_link_libraries(vncconfig rfb_win32 rfb network rdr ws2_32.lib)

install(TARGETS vncconfig
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
