add_library(network STATIC
  Socket.cxx
  TcpSocket.cxx)

if(NOT WIN32)
  target_sources(network PRIVATE UnixSocket.cxx)
endif()

target_include_directories(network PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(network os rdr rfb)

if(WIN32)
	target_link_libraries(network ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(network)
endif()
