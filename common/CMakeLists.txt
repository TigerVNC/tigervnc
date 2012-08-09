if(USE_INCLUDED_ZLIB)
  add_subdirectory(zlib)

  set(ZLIB_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/common/zlib)
  set(ZLIB_LIBRARIES zlib)
endif()

add_subdirectory(os)
add_subdirectory(rdr)
add_subdirectory(network)
add_subdirectory(Xregion)
add_subdirectory(rfb)

# For any convenience libraries that are linked into libvnc.so, we need to
# explicitly build their corresponding sources using PIC.  WIN32 is excluded
# because PIC code does not exist on that platform and MinGW complains if -fPIC
# is passed (additionally, libvnc is not used on Windows.)

if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_SIZEOF_VOID_P MATCHES 8) AND NOT WIN32)
  set_target_properties(os rdr network Xregion rfb
    PROPERTIES COMPILE_FLAGS -fPIC)
  if(USE_INCLUDED_ZLIB)
    set_target_properties(zlib PROPERTIES COMPILE_FLAGS -fPIC)
  endif()
endif()
