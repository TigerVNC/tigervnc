include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(os STATIC
  print.c
  net.c
  os.cxx)
