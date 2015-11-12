include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(os STATIC
  Mutex.cxx
  Thread.cxx
  w32tiger.c
  os.cxx)

if(UNIX)
  target_link_libraries(os pthread)
endif()

if(UNIX)
  libtool_create_control_file(os)
endif()
