include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(network STATIC
  TcpSocket.cxx)

if(WIN32)
	target_link_libraries(network ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(network)
endif()
