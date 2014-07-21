include_directories(${CMAKE_BINARY_DIR}/win)

add_executable(vncconfig WIN32
  Legacy.cxx
  PasswordDialog.cxx
  vncconfig.cxx
  vncconfig.rc)

# When building with GnuTLS, librdr depends on ws2_32, so in order to make
# MinGW happy, we need to put ws2_32 in librdr's target_link_libraries string,
# not here.
if(GNUTLS_FOUND)
  target_link_libraries(vncconfig rfb_win32 rfb Xregion network rdr)
else()
  target_link_libraries(vncconfig rfb_win32 rfb Xregion network rdr ${WINSOCK2_LIBRARIES})
endif()

install(TARGETS vncconfig
  RUNTIME DESTINATION ${BIN_DIR}
)
