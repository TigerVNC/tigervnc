if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_library(rfb STATIC
  AccessRights.cxx
  Blacklist.cxx
  Congestion.cxx
  ComparingUpdateTracker.cxx
  Cursor.cxx
  d3des.c
  JpegCompressor.cxx
  JpegDecompressor.cxx
  KeyRemapper.cxx
  KeysymStr.c
  PixelBuffer.cxx
  PixelFormat.cxx
  Security.cxx
  UpdateTracker.cxx
  encodings.cxx
  obfuscate.cxx)

target_include_directories(rfb PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(rfb SYSTEM PUBLIC ${JPEG_INCLUDE_DIR})
target_link_libraries(rfb core rdr)
target_link_libraries(rfb ${JPEG_LIBRARIES})

add_library(rfbserver STATIC
  ClientParams.cxx
  EncodeManager.cxx
  Encoder.cxx
  HextileEncoder.cxx
  JPEGEncoder.cxx
  RREEncoder.cxx
  RawEncoder.cxx
  SConnection.cxx
  SMsgReader.cxx
  SMsgWriter.cxx
  ServerCore.cxx
  SecurityServer.cxx
  SSecurityPlain.cxx
  SSecurityStack.cxx
  SSecurityVncAuth.cxx
  SSecurityVeNCrypt.cxx
  TightEncoder.cxx
  TightJPEGEncoder.cxx
  VNCSConnectionST.cxx
  VNCServerST.cxx
  ZRLEEncoder.cxx)

target_include_directories(rfbserver PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(rfbserver core rdr network rfb)

add_library(rfbclient STATIC
  CConnection.cxx
  CMsgReader.cxx
  CMsgWriter.cxx
  CSecurityPlain.cxx
  CSecurityStack.cxx
  CSecurityVeNCrypt.cxx
  CSecurityVncAuth.cxx
  CopyRectDecoder.cxx
  DecodeManager.cxx
  Decoder.cxx
  HextileDecoder.cxx
  JPEGDecoder.cxx
  RREDecoder.cxx
  RawDecoder.cxx
  SecurityClient.cxx
  ServerParams.cxx
  TightDecoder.cxx
  ZRLEDecoder.cxx)

target_include_directories(rfbclient PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(rfbclient core rdr rfb)

if(ENABLE_H264)
  target_sources(rfbclient PRIVATE
                 H264Decoder.cxx H264DecoderContext.cxx)
  if (AVCODEC_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND)
    target_sources(rfbclient PRIVATE H264LibavDecoderContext.cxx)
    target_include_directories(rfbclient SYSTEM PUBLIC
                               ${AVCODEC_INCLUDE_DIRS}
                               ${AVUTIL_INCLUDE_DIRS}
                               ${SWSCALE_INCLUDE_DIRS})
    target_link_libraries(rfbclient ${AVCODEC_LIBRARIES}
                          ${AVUTIL_LIBRARIES} ${SWSCALE_LIBRARIES})
  endif()
  if(WIN32)
    target_sources(rfbclient PRIVATE H264WinDecoderContext.cxx)
    target_link_libraries(rfbclient ole32 mfplat mfuuid wmcodecdspuuid)
  endif()
  target_include_directories(rfbclient SYSTEM PUBLIC ${H264_INCLUDE_DIRS})
  target_link_libraries(rfbclient ${H264_LIBRARIES})
endif()

if(WIN32)
  target_include_directories(rfbserver PUBLIC ${CMAKE_SOURCE_DIR}/win)
  target_sources(rfbserver PRIVATE WinPasswdValidator.cxx)
endif(WIN32)

if(PAM_FOUND)
  target_sources(rfbserver PRIVATE UnixPasswordValidator.cxx)
  target_include_directories(rfbserver SYSTEM PRIVATE ${PAM_INCLUDE_DIRS})
  target_link_libraries(rfbserver ${PAM_LIBRARIES})
endif()

if(GNUTLS_FOUND)
  target_sources(rfbserver PRIVATE SSecurityTLS.cxx)
  target_sources(rfbclient PRIVATE CSecurityTLS.cxx)
  target_include_directories(rfbserver SYSTEM PUBLIC ${GNUTLS_INCLUDE_DIR})
  target_include_directories(rfbclient SYSTEM PUBLIC ${GNUTLS_INCLUDE_DIR})
  target_link_libraries(rfbserver ${GNUTLS_LIBRARIES})
  target_link_libraries(rfbclient ${GNUTLS_LIBRARIES})
  # FIXME: Hack to block it marking gnutls_free() as dllimport
  if(WIN32 AND BUILD_STATIC)
    target_compile_definitions(rfbserver PRIVATE GNUTLS_INTERNAL_BUILD)
    target_compile_definitions(rfbclient PRIVATE GNUTLS_INTERNAL_BUILD)
  endif()
endif()

if (NETTLE_FOUND)
  target_sources(rfbserver PRIVATE SSecurityRSAAES.cxx)
  target_sources(rfbclient PRIVATE CSecurityDH.cxx
                 CSecurityMSLogonII.cxx CSecurityRSAAES.cxx)
  target_include_directories(rfbserver SYSTEM PUBLIC ${NETTLE_INCLUDE_DIRS})
  target_include_directories(rfbclient SYSTEM PUBLIC ${NETTLE_INCLUDE_DIRS})
  target_link_libraries(rfbserver ${HOGWEED_LIBRARIES} ${NETTLE_LIBRARIES})
  target_link_libraries(rfbclient ${HOGWEED_LIBRARIES} ${NETTLE_LIBRARIES})
endif()

if(UNIX)
  libtool_create_control_file(rfb)
  libtool_create_control_file(rfbserver)
  libtool_create_control_file(rfbclient)
endif()
