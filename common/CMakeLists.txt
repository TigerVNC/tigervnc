if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_subdirectory(core)
add_subdirectory(rdr)
add_subdirectory(network)
add_subdirectory(rfb)

# For any convenience libraries that are linked into libvnc.so, we need to
# explicitly build their corresponding sources using PIC.  WIN32 is excluded
# because PIC code does not exist on that platform and MinGW complains if -fPIC
# is passed (additionally, libvnc is not used on Windows.)

if(NOT WIN32)
  set_target_properties(core rdr network rfb rfbserver
    PROPERTIES COMPILE_FLAGS -fPIC)
endif()
