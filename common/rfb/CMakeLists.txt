include_directories(${CMAKE_SOURCE_DIR}/common ${JPEG_INCLUDE_DIR})

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
  JpegCompressor.cxx
  KeyRemapper.cxx
  LogWriter.cxx
  Logger.cxx
  Logger_file.cxx
  Logger_stdio.cxx
  Password.cxx
  PixelBuffer.cxx
  PixelFormat.cxx
  PixelTransformer.cxx
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
  include_directories(${CMAKE_SOURCE_DIR}/win)
  set(RFB_SOURCES ${RFB_SOURCES} WinPasswdValidator.cxx)
endif(WIN32)

set(RFB_LIBRARIES ${JPEG_LIBRARIES} os)

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

# This code goes into libvnc.so, so it needs to be PIC on x86_64.
# (It should be on archs as well, but we prefer performance over memory
# efficiency. Win64 is excluded because PIC is always on there and it
# complains when you give it the explicit flag.)
if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_SIZEOF_VOID_P MATCHES 8) AND NOT WIN32)
  set_target_properties(rfb PROPERTIES COMPILE_FLAGS -fPIC)
endif()

if(UNIX)
  libtool_create_control_file(rfb)
endif()
