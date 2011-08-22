include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(network STATIC
  TcpSocket.cxx)

# This code goes into libvnc.so, so it needs to be PIC on x86_64.
# (It should be on archs as well, but we prefer performance over memory
# efficiency. Win64 is excluded because PIC is always on there and it
# complains when you give it the explicit flag.)
if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_SIZEOF_VOID_P MATCHES 8) AND NOT WIN32)
  set_target_properties(network PROPERTIES COMPILE_FLAGS -fPIC)
endif()

if(UNIX)
  libtool_create_control_file(network)
endif()
