include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(os STATIC
  Mutex.cxx
  Thread.cxx
  os.cxx)

target_link_libraries(os rdr)

if(UNIX)
  target_link_libraries(os pthread)
endif()

if(UNIX)
  libtool_create_control_file(os)
endif()
