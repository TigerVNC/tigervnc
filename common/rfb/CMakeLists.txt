if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_library(rfb STATIC
  AccessRights.cxx
  Blacklist.cxx
  Congestion.cxx
  CConnection.cxx
  CMsgReader.cxx
  CMsgWriter.cxx
  CSecurityPlain.cxx
  CSecurityStack.cxx
  CSecurityVeNCrypt.cxx
  CSecurityVncAuth.cxx
  ClientParams.cxx
  ComparingUpdateTracker.cxx
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
  JPEGDecoder.cxx
  JPEGEncoder.cxx
  KeyRemapper.cxx
  KeysymStr.c
  PixelBuffer.cxx
  PixelFormat.cxx
  RREEncoder.cxx
  RREDecoder.cxx
  RawDecoder.cxx
  RawEncoder.cxx
  SConnection.cxx
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
  TightDecoder.cxx
  TightEncoder.cxx
  TightJPEGEncoder.cxx
  UpdateTracker.cxx
  VNCSConnectionST.cxx
  VNCServerST.cxx
  ZRLEEncoder.cxx
  ZRLEDecoder.cxx
  encodings.cxx
  obfuscate.cxx)

target_include_directories(rfb PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(rfb SYSTEM PUBLIC ${JPEG_INCLUDE_DIR})
target_link_libraries(rfb core rdr network)
target_link_libraries(rfb ${JPEG_LIBRARIES} ${PIXMAN_LIBRARIES})

if(ENABLE_H264)
  target_sources(rfb PRIVATE H264Decoder.cxx H264DecoderContext.cxx)
  if (AVCODEC_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND)
    target_sources(rfb PRIVATE H264LibavDecoderContext.cxx)
    target_include_directories(rfb SYSTEM PUBLIC
                               ${AVCODEC_INCLUDE_DIRS}
                               ${AVUTIL_INCLUDE_DIRS}
                               ${SWSCALE_INCLUDE_DIRS})
    target_link_libraries(rfb ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES}
                          ${SWSCALE_LIBRARIES})
  endif()
  if(WIN32)
    target_sources(rfb PRIVATE H264WinDecoderContext.cxx)
    target_link_libraries(rfb ole32 mfplat mfuuid wmcodecdspuuid)
  endif()
  target_include_directories(rfb SYSTEM PUBLIC ${H264_INCLUDE_DIRS})
  target_link_libraries(rfb ${H264_LIBRARIES})
endif()

if(WIN32)
  target_include_directories(rfb PUBLIC ${CMAKE_SOURCE_DIR}/win)
  target_sources(rfb PRIVATE WinPasswdValidator.cxx)
endif(WIN32)

if(UNIX AND NOT APPLE)
  target_sources(rfb PRIVATE UnixPasswordValidator.cxx)
  target_include_directories(rfb SYSTEM PRIVATE ${PAM_INCLUDE_DIRS})
  target_link_libraries(rfb ${PAM_LIBRARIES})
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
  target_include_directories(rfb SYSTEM PUBLIC ${NETTLE_INCLUDE_DIRS})
  target_link_libraries(rfb ${HOGWEED_LIBRARIES} ${NETTLE_LIBRARIES})
endif()

if(UNIX)
  libtool_create_control_file(rfb)
endif()
