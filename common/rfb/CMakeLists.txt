add_library(rfb STATIC
  AccessRights.cxx
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
  KeysymStr.c
  LogWriter.cxx
  Logger.cxx
  Logger_file.cxx
  Logger_stdio.cxx
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
  obfuscate.cxx
  util.cxx)

target_include_directories(rfb PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(rfb SYSTEM PUBLIC ${JPEG_INCLUDE_DIR})
target_include_directories(rfb SYSTEM PUBLIC ${PIXMAN_INCLUDE_DIRS})
target_link_libraries(rfb os rdr network)
target_link_libraries(rfb ${JPEG_LIBRARIES} ${PIXMAN_LIBRARIES})
target_link_directories(rfb PUBLIC ${PIXMAN_LIBRARY_DIRS})

if(ENABLE_H264 AND NOT H264_LIBS STREQUAL "NONE")
  target_sources(rfb PRIVATE H264Decoder.cxx H264DecoderContext.cxx)
  if(H264_LIBS STREQUAL "LIBAV")
    target_sources(rfb PRIVATE H264LibavDecoderContext.cxx)
  elseif(H264_LIBS STREQUAL "WIN")
    target_sources(rfb PRIVATE H264WinDecoderContext.cxx)
  endif()
  target_include_directories(rfb SYSTEM PUBLIC ${H264_INCLUDE_DIRS})
  target_link_libraries(rfb ${H264_LIBRARIES})
  target_link_directories(rfb PUBLIC ${H264_LIBRARY_DIRS})
endif()

if(UNIX)
  target_sources(rfb PRIVATE Logger_syslog.cxx)
endif()

if(WIN32)
  target_include_directories(rfb PUBLIC ${CMAKE_SOURCE_DIR}/win)
  target_sources(rfb PRIVATE WinPasswdValidator.cxx)
endif(WIN32)

if(UNIX AND NOT APPLE)
  target_sources(rfb PRIVATE UnixPasswordValidator.cxx pam.c)
  target_link_libraries(rfb ${PAM_LIBS})
endif()

if(GNUTLS_FOUND)
  target_sources(rfb PRIVATE CSecurityTLS.cxx SSecurityTLS.cxx)
  target_include_directories(rfb SYSTEM PUBLIC ${GNUTLS_INCLUDE_DIR})
  target_link_libraries(rfb ${GNUTLS_LIBRARIES})
  # FIXME: Hack to block it marking gnutls_free() as dllimport
  if(WIN32 AND BUILD_STATIC)
    target_compile_definitions(rfb PRIVATE GNUTLS_INTERNAL_BUILD)
  endif()
endif()

if (NETTLE_FOUND)
  target_sources(rfb PRIVATE CSecurityDH.cxx CSecurityMSLogonII.cxx
                 CSecurityRSAAES.cxx SSecurityRSAAES.cxx)
  target_include_directories(rfb SYSTEM PUBLIC ${NETTLE_INCLUDE_DIRS}
                             ${GMP_INCLUDE_DIRS})
  target_link_libraries(rfb ${HOGWEED_LIBRARIES}
                        ${NETTLE_LIBRARIES} ${GMP_LIBRARIES})
  target_link_directories(rfb PUBLIC ${HOGWEED_LIBRARY_DIRS}
                          ${NETTLE_LIBRARY_DIRS} ${GMP_LIBRARY_DIRS})
endif()

if(UNIX)
  libtool_create_control_file(rfb)
endif()
