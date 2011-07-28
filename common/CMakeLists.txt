if(USE_INCLUDED_ZLIB)
  add_subdirectory(zlib)

  set(ZLIB_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/common/zlib)
  set(ZLIB_LIBRARIES zlib)
endif()

if(USE_INCLUDED_FLTK)
  add_subdirectory(fltk)
endif()

add_subdirectory(os)
add_subdirectory(rdr)
add_subdirectory(network)
add_subdirectory(Xregion)
add_subdirectory(rfb)
