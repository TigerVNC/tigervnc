if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_library(network STATIC
  DnsSD.cxx
  Socket.cxx
  TcpSocket.cxx)

if(NOT WIN32)
  target_sources(network PRIVATE UnixSocket.cxx)
endif()

target_include_directories(network PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(network core rdr)

if(HAVE_AVAHI_CLIENT)
  target_include_directories(network PRIVATE ${AVAHI_CLIENT_INCLUDE_DIRS})
  target_link_libraries(network ${AVAHI_CLIENT_LIBRARIES})
endif()

if(WIN32)
	target_link_libraries(network ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(network)
endif()
