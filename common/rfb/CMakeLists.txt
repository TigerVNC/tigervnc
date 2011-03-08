include_directories(${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/win ${JPEG_INCLUDE_DIR})

set(RFB_SOURCES
  Blacklist.cxx
  CConnection.cxx
  CMsgHandler.cxx
  CMsgReader.cxx
  CMsgReaderV3.cxx
  CMsgWriter.cxx
  CMsgWriterV3.cxx
  CSecurityPlain.cxx
  CSecurityStack.cxx
  CSecurityVeNCrypt.cxx
  CSecurityVncAuth.cxx
  ComparingUpdateTracker.cxx
  Configuration.cxx
  ConnParams.cxx
  Cursor.cxx
  Decoder.cxx
  d3des.c
  Encoder.cxx
  HTTPServer.cxx
  HextileDecoder.cxx
  HextileEncoder.cxx
  KeyRemapper.cxx
  LogWriter.cxx
  Logger.cxx
  Logger_file.cxx
  Logger_stdio.cxx
  Password.cxx
  PixelBuffer.cxx
  PixelFormat.cxx
  RREEncoder.cxx
  RREDecoder.cxx
  RawDecoder.cxx
  RawEncoder.cxx
  Region.cxx
  SConnection.cxx
  SMsgHandler.cxx
  SMsgReader.cxx
  SMsgReaderV3.cxx
  SMsgWriter.cxx
  SMsgWriterV3.cxx
  ServerCore.cxx
  Security.cxx
  SecurityServer.cxx
  SecurityClient.cxx
  SSecurityPlain.cxx
  SSecurityStack.cxx
  SSecurityVncAuth.cxx
  SSecurityVeNCrypt.cxx
  ScaledPixelBuffer.cxx
  ScaleFilters.cxx
  Timer.cxx
  TightDecoder.cxx
  TightEncoder.cxx
  TightPalette.cxx
  TransImageGetter.cxx
  UpdateTracker.cxx
  VNCSConnectionST.cxx
  VNCServerST.cxx
  ZRLEEncoder.cxx
  ZRLEDecoder.cxx
  encodings.cxx
  util.cxx)

if(WIN32)
  set(RFB_SOURCES ${RFB_SOURCES} WinPasswdValidator.cxx)
endif(WIN32)

set(RFB_LIBRARIES ${JPEG_LIBRARIES} os)

if(GNUTLS_FOUND)
  set(RFB_SOURCES
    ${RFB_SOURCES}
    CSecurityTLS.cxx
    SSecurityTLS.cxx
  )
  set(RFB_LIBRARIES
    ${RFB_LIBRARIES}
    ${GNUTLS_LIBRARIES}
  )
endif()

add_library(rfb STATIC ${RFB_SOURCES})

target_link_libraries(rfb ${RFB_LIBRARIES})
