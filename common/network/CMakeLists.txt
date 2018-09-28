include_directories(${CMAKE_SOURCE_DIR}/common)

set(NETWORK_SOURCES
  Socket.cxx
  TcpSocket.cxx)

if(NOT WIN32)
  set(NETWORK_SOURCES ${NETWORK_SOURCES} UnixSocket.cxx)
endif()

add_library(network STATIC ${NETWORK_SOURCES})

if(WIN32)
	target_link_libraries(network ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(network)
endif()
