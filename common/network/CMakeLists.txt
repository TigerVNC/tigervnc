include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(network STATIC
  TcpSocket.cxx)

if(UNIX)
  libtool_create_control_file(network)
endif()
