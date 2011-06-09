include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(network STATIC
  TcpSocket.cxx)

libtool_create_control_file(network)
