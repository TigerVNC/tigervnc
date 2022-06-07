include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(network STATIC
  Socket.cxx
  TcpSocket.cxx)

if(NOT WIN32)
  target_sources(network PRIVATE UnixSocket.cxx)
endif()

if(WIN32)
	target_link_libraries(network ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(network)
endif()
