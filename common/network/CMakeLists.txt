file(GLOB HEAD_FILES *.h)
set(INSTALL_HEAD_FILES ${HEAD_FILES})

add_library(network OBJECT
  ${INSTALL_HEAD_FILES}
  Socket.cxx
  TcpSocket.cxx)

if(NOT WIN32)
  target_sources(network PRIVATE UnixSocket.cxx)
endif()

target_include_directories(network PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)
target_link_libraries(network rdr)

if(WIN32)
  target_link_libraries(network ws2_32)
endif()

# Set install head files
set_target_properties(network PROPERTIES
  PUBLIC_HEADER "${INSTALL_HEAD_FILES}")
# Install head files
INSTALL(TARGETS network
  EXPORT tigervncConfig
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc/network
    COMPONENT Development
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc
  )
