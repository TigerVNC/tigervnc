add_subdirectory(os)
add_subdirectory(rdr)
add_subdirectory(network)
add_subdirectory(rfb)

# For any convenience libraries that are linked into libvnc.so, we need to
# explicitly build their corresponding sources using PIC.  WIN32 is excluded
# because PIC code does not exist on that platform and MinGW complains if -fPIC
# is passed (additionally, libvnc is not used on Windows.)

if(NOT WIN32)
  set_target_properties(os rdr network rfb
    PROPERTIES COMPILE_FLAGS -fPIC)
endif()
