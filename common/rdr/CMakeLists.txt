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

target_include_directories(rdr PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(rdr SYSTEM PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_libraries(rdr ${ZLIB_LIBRARIES} os rfb)

if(MSVC)
  # undef min and max macro
  target_compile_definitions(rfb PRIVATE NOMINMAX)
endif()

if(GNUTLS_FOUND)
  target_include_directories(rdr SYSTEM PUBLIC ${GNUTLS_INCLUDE_DIR})
  target_link_libraries(rdr ${GNUTLS_LIBRARIES})
endif()
if (NETTLE_FOUND)
  target_include_directories(rdr SYSTEM PUBLIC ${NETTLE_INCLUDE_DIRS})
  target_link_libraries(rdr ${NETTLE_LIBRARIES})
  target_link_directories(rdr PUBLIC ${NETTLE_LIBRARY_DIRS})
endif()
if(WIN32)
	target_link_libraries(rdr ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(rdr)
endif()
