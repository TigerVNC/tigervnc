include_directories(${CMAKE_BINARY_DIR}/win)

add_executable(vncconfig WIN32
  Legacy.cxx
  PasswordDialog.cxx
  vncconfig.cxx
  vncconfig.rc)

target_link_libraries(vncconfig rfb_win32 rfb network rdr ws2_32.lib)

install(TARGETS vncconfig
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
