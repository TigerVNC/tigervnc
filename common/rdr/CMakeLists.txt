include_directories(${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/common/zlib)

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

set(RDR_LIBRARIES zlib os)
if(GNUTLS_FOUND)
  set(RDR_LIBRARIES ${RDR_LIBRARIES} ${GNUTLS_LIBRARIES} ws2_32)
endif()

target_link_libraries(rdr ${RDR_LIBRARIES})
