include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(os STATIC
  Mutex.cxx
  Thread.cxx
  w32tiger.c
  os.cxx)

if(UNIX)
  libtool_create_control_file(os)
endif()
