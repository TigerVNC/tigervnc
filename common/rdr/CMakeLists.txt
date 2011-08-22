include_directories(${CMAKE_SOURCE_DIR}/common ${ZLIB_INCLUDE_DIRS})

add_library(rdr STATIC
  Exception.cxx
  FdInStream.cxx
  FdOutStream.cxx
  HexInStream.cxx
  HexOutStream.cxx
  InStream.cxx
  RandomStream.cxx
  TLSException.cxx
  TLSInStream.cxx
  TLSOutStream.cxx
  ZlibInStream.cxx
  ZlibOutStream.cxx)

set(RDR_LIBRARIES ${ZLIB_LIBRARIES} os)
if(GNUTLS_FOUND)
  set(RDR_LIBRARIES ${RDR_LIBRARIES} ${GNUTLS_LIBRARIES})
  if(WIN32)
	set(RDR_LIBRARIES ${RDR_LIBRARIES} ws2_32)
  endif()
endif()

target_link_libraries(rdr ${RDR_LIBRARIES})

# This code goes into libvnc.so, so it needs to be PIC on x86_64.
# (It should be on archs as well, but we prefer performance over memory
# efficiency. Win64 is excluded because PIC is always on there and it
# complains when you give it the explicit flag.)
if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_SIZEOF_VOID_P MATCHES 8) AND NOT WIN32)
  set_target_properties(rdr PROPERTIES COMPILE_FLAGS -fPIC)
endif()

if(UNIX)
  libtool_create_control_file(rdr)
endif()
