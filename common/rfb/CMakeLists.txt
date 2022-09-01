include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${JPEG_INCLUDE_DIR})
include_directories(${PIXMAN_INCLUDE_DIRS})

add_library(rfb STATIC
  Blacklist.cxx
  Congestion.cxx
  CConnection.cxx
  CMsgHandler.cxx
  CMsgReader.cxx
  CMsgWriter.cxx
  CSecurityPlain.cxx
  CSecurityStack.cxx
  CSecurityVeNCrypt.cxx
  CSecurityVncAuth.cxx
  ClientParams.cxx
  ComparingUpdateTracker.cxx
  Configuration.cxx
  CopyRectDecoder.cxx
  Cursor.cxx
  DecodeManager.cxx
  Decoder.cxx
  d3des.c
  EncodeManager.cxx
  Encoder.cxx
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
  ServerParams.cxx
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

target_link_libraries(rfb os rdr)
target_link_libraries(rfb ${JPEG_LIBRARIES} ${PIXMAN_LIBRARIES})
target_link_directories(rfb PUBLIC ${PIXMAN_LIBRARY_DIRS})

if(ENABLE_H264 AND NOT H264_LIBS STREQUAL "NONE")
  target_sources(rfb PRIVATE H264Decoder.cxx H264DecoderContext.cxx)
  if(H264_LIBS STREQUAL "LIBAV")
    target_sources(rfb PRIVATE H264LibavDecoderContext.cxx)
  elseif(H264_LIBS STREQUAL "WIN")
    target_sources(rfb PRIVATE H264WinDecoderContext.cxx)
  endif()
  include_directories(${H264_INCLUDE_DIRS})
  target_link_libraries(rfb ${H264_LIBRARIES})
  target_link_directories(rfb PUBLIC ${H264_LIBRARY_DIRS})
endif()

if(UNIX)
  target_sources(rfb PRIVATE Logger_syslog.cxx)
endif()

if(WIN32)
  include_directories(${CMAKE_SOURCE_DIR}/win)
  target_sources(rfb PRIVATE WinPasswdValidator.cxx)
endif(WIN32)

if(UNIX AND NOT APPLE)
  target_sources(rfb PRIVATE UnixPasswordValidator.cxx pam.c)
  target_link_libraries(rfb ${PAM_LIBS})
endif()

if(GNUTLS_FOUND)
  target_sources(rfb PRIVATE CSecurityTLS.cxx SSecurityTLS.cxx)
  include_directories(${GNUTLS_INCLUDE_DIR})
  target_link_libraries(rfb ${GNUTLS_LIBRARIES})
endif()

if (NETTLE_FOUND)
  target_sources(rfb PRIVATE CSecurityRSAAES.cxx SSecurityRSAAES.cxx)
  include_directories(${NETTLE_INCLUDE_DIRS} ${GMP_INCLUDE_DIRS})
  target_link_libraries(rfb ${HOGWEED_LIBRARIES}
                        ${NETTLE_LIBRARIES} ${GMP_LIBRARIES})
  target_link_directories(rfb PUBLIC ${HOGWEED_LIBRARY_DIRS}
                          ${NETTLE_LIBRARY_DIRS} ${GMP_LIBRARY_DIRS})
endif()

if(UNIX)
  libtool_create_control_file(rfb)
endif()
