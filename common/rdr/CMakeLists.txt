add_library(rdr STATIC
  AESInStream.cxx
  AESOutStream.cxx
  BufferedInStream.cxx
  BufferedOutStream.cxx
  FdInStream.cxx
  FdOutStream.cxx
  FileInStream.cxx
  HexInStream.cxx
  HexOutStream.cxx
  RandomStream.cxx
  TLSException.cxx
  TLSInStream.cxx
  TLSOutStream.cxx
  TLSSocket.cxx
  ZlibInStream.cxx
  ZlibOutStream.cxx)

target_include_directories(rdr PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(rdr SYSTEM PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_libraries(rdr core)
target_link_libraries(rdr ${ZLIB_LIBRARIES})

if(GNUTLS_FOUND)
  target_include_directories(rdr SYSTEM PUBLIC ${GNUTLS_INCLUDE_DIR})
  target_link_libraries(rdr ${GNUTLS_LIBRARIES})
endif()
if (NETTLE_FOUND)
  target_include_directories(rdr SYSTEM PUBLIC ${NETTLE_INCLUDE_DIRS})
  target_link_libraries(rdr ${NETTLE_LIBRARIES})
endif()
if(WIN32)
	target_link_libraries(rdr ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(rdr)
endif()
