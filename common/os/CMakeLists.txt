include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(os STATIC
  print.c
  net.c
  w32tiger.c
  os.cxx
  tls.cxx)

if(UNIX)
  libtool_create_control_file(os)
endif()
