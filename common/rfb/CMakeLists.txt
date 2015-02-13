include_directories(${CMAKE_SOURCE_DIR}/common ${JPEG_INCLUDE_DIR})

set(RFB_SOURCES
  Blacklist.cxx
  CConnection.cxx
  CMsgHandler.cxx
  CMsgReader.cxx
  CMsgWriter.cxx
  CSecurityPlain.cxx
  CSecurityStack.cxx
  CSecurityVeNCrypt.cxx
  CSecurityVncAuth.cxx
  ComparingUpdateTracker.cxx
  Configuration.cxx
  ConnParams.cxx
  CopyRectDecoder.cxx
  Cursor.cxx
  Decoder.cxx
  d3des.c
  EncodeManager.cxx
  Encoder.cxx
  HTTPServer.cxx
  HextileDecoder.cxx
  HextileEncoder.cxx
  JpegCompressor.cxx
  JpegDecompressor.cxx
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
  SMsgWriter.cxx
  ServerCore.cxx
  Security.cxx
  SecurityServer.cxx
  SecurityClient.cxx
  SSecurityPlain.cxx
  SSecurityStack.cxx
  SSecurityVncAuth.cxx
  SSecurityVeNCrypt.cxx
  ScaleFilters.cxx
  Timer.cxx
  TightDecoder.cxx
  TightEncoder.cxx
  TightJPEGEncoder.cxx
  UpdateTracker.cxx
  VNCSConnectionST.cxx
  VNCServerST.cxx
  ZRLEEncoder.cxx
  ZRLEDecoder.cxx
  encodings.cxx
  util.cxx)

if(WIN32)
  include_directories(${CMAKE_SOURCE_DIR}/win)
  set(RFB_SOURCES ${RFB_SOURCES} WinPasswdValidator.cxx)
endif(WIN32)

set(RFB_LIBRARIES ${JPEG_LIBRARIES} os rdr Xregion)

if(HAVE_PAM)
  set(RFB_SOURCES ${RFB_SOURCES} UnixPasswordValidator.cxx
    UnixPasswordValidator.h pam.c pam.h)
  set(RFB_LIBRARIES ${RFB_LIBRARIES} ${PAM_LIBS})
endif()

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

if(UNIX)
  libtool_create_control_file(rfb)
endif()
