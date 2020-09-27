project(tigervnc)

# Check for GNUTLS library
option(ENABLE_GNUTLS "Enable protocol encryption and advanced authentication" ON)
if(ENABLE_GNUTLS)
  find_package(GnuTLS)
endif()

add_subdirectory(os)
add_subdirectory(rdr)
add_subdirectory(network)
add_subdirectory(rfb)

set(tigervnc_FIND_COMPONENTS os rdr network rfb)
CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/cmake/tigervncConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/tigervncConfig.cmake"
    IMMEDIATE @ONLY)
INSTALL(FILES "${CMAKE_BINARY_DIR}/tigervncConfig.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tigervnc")
