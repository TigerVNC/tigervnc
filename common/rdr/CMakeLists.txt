include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${ZLIB_INCLUDE_DIRS})

add_library(rdr STATIC
  AESInStream.cxx
  AESOutStream.cxx
  BufferedInStream.cxx
  BufferedOutStream.cxx
  Exception.cxx
  FdInStream.cxx
  FdOutStream.cxx
  FileInStream.cxx
  HexInStream.cxx
  HexOutStream.cxx
  RandomStream.cxx
  TLSException.cxx
  TLSInStream.cxx
  TLSOutStream.cxx
  ZlibInStream.cxx
  ZlibOutStream.cxx)

target_link_libraries(rdr ${ZLIB_LIBRARIES} os)

if(GNUTLS_FOUND)
  include_directories(${GNUTLS_INCLUDE_DIR})
  target_link_libraries(rdr ${GNUTLS_LIBRARIES})
endif()
if (NETTLE_FOUND)
  include_directories(${NETTLE_INCLUDE_DIRS})
  target_link_libraries(rdr ${NETTLE_LIBRARIES})
  target_link_directories(rdr PUBLIC ${NETTLE_LIBRARY_DIRS})
endif()
if(WIN32)
	target_link_libraries(rdr ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(rdr)
endif()
