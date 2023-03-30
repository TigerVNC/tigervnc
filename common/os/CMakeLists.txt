add_library(os STATIC
  Mutex.cxx
  Thread.cxx
  os.cxx)

target_include_directories(os PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(os rdr)

if(UNIX)
  target_link_libraries(os pthread)
endif()

if(UNIX)
  libtool_create_control_file(os)
endif()
