add_subdirectory(core)
add_subdirectory(rdr)
add_subdirectory(network)
add_subdirectory(rfb)

# For any convenience libraries that are linked into libvnc.so, we need to
# explicitly build their corresponding sources using PIC.  WIN32 is excluded
# because PIC code does not exist on that platform and MinGW complains if -fPIC
# is passed (additionally, libvnc is not used on Windows.)

if(NOT WIN32)
  set_target_properties(core rdr network rfb
    PROPERTIES COMPILE_FLAGS -fPIC)
endif()

add_library(tigervnc STATIC
    $<TARGET_OBJECTS:core>
    $<TARGET_OBJECTS:rdr>
    $<TARGET_OBJECTS:network>
    $<TARGET_OBJECTS:rfb>)
target_link_libraries(tigervnc PUBLIC rfb)

# cmake version configure file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/tigervncConfigVersion.cmake"
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tigervncConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tigervnc"
    COMPONENT Development
    )

# Generate an export header file, which is required when generating DLLs using msvc
GENERATE_EXPORT_HEADER(tigervnc)
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tigervnc_export.h
    DESTINATION ${CMAKE_BINARY_DIR})
list(APPEND INSTALL_HEAD_FILES ${CMAKE_CURRENT_BINARY_DIR}/tigervnc_export.h)
set_target_properties(tigervnc PROPERTIES
    PUBLIC_HEADER "${INSTALL_HEAD_FILES}" # Install head files
    )

# Install head files and library
INSTALL(TARGETS tigervnc
    EXPORT tigervncConfig
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc
      COMPONENT Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc
    )
install(EXPORT tigervncConfig
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tigervnc"
      COMPONENT Development
    NAMESPACE tigervnc::
    )

if(UNIX)
  libtool_create_control_file(tigervnc)
endif()
