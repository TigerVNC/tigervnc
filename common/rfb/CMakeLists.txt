# Check for pixman
find_package(Pixman REQUIRED)

set(RFB_INCLUDE_DIR ${RFB_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
    ${PIXMAN_INCLUDE_DIRS}
    )

set(RFB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Blacklist.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Congestion.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CConnection.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CMsgHandler.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CMsgReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CMsgWriter.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityPlain.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityStack.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityVeNCrypt.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityVncAuth.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientParams.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ComparingUpdateTracker.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Configuration.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/CopyRectDecoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Cursor.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/DecodeManager.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Decoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/d3des.c
    ${CMAKE_CURRENT_SOURCE_DIR}/EncodeManager.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Encoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/HextileDecoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/HextileEncoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/JpegCompressor.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/JpegDecompressor.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/KeyRemapper.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/LogWriter.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger_file.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger_stdio.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Password.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelBuffer.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelFormat.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/RREEncoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/RREDecoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/RawDecoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/RawEncoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Region.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SConnection.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SMsgHandler.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SMsgReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SMsgWriter.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerCore.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerParams.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Security.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SecurityServer.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SecurityClient.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityPlain.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityStack.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityVncAuth.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityVeNCrypt.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ScaleFilters.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Timer.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/TightDecoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/TightEncoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/TightJPEGEncoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/UpdateTracker.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/VNCSConnectionST.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/VNCServerST.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ZRLEEncoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ZRLEDecoder.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/encodings.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/util.cxx)

set(RFB_HEADS ${RFB_HEADS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Blacklist.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Congestion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CConnection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clipboardTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CMsgHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CMsgReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CMsgWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityNone.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityPlain.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityStack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityVeNCrypt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityVncAuth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientParams.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ComparingUpdateTracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Configuration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CopyRectDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Cursor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DecodeManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/d3des.c
    ${CMAKE_CURRENT_SOURCE_DIR}/EncodeManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hextileDecode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HextileDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hextileEncode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HextileEncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/JpegCompressor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/JpegDecompressor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/KeyRemapper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/keysymdef.h
    ${CMAKE_CURRENT_SOURCE_DIR}/XF86keysym.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ledStates.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger_file.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger_stdio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Password.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Pixel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelBuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelFormat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rreEncode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RREEncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rreDecode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RREDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RawDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RawEncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Region.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SConnection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SMsgHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SMsgReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SMsgWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerCore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerParams.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Security.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SecurityServer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SecurityClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityNone.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityPlain.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityTLS.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityStack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityVncAuth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityVeNCrypt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScaleFilters.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Timer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tightDecode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TightDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TightEncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TightJPEGEncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UpdateTracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VNCSConnectionST.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VNCServerST.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zrleDecode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ZRLEEncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ZRLEDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encodings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UserPasswdGetter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fenceTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hextileConstants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/msgTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Palette.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Rect.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScreenSet.h
    ${CMAKE_CURRENT_SOURCE_DIR}/screenTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SDesktop.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TightConstants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelFormat.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/Exception.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Hostname.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UserMsgBox.h
    )

if(UNIX)
    set(RFB_SOURCES ${RFB_SOURCES} 
        ${CMAKE_CURRENT_SOURCE_DIR}/Logger_syslog.cxx)
    set(RFB_HEADS ${RFB_HEADS}
        ${CMAKE_CURRENT_SOURCE_DIR}/Logger_syslog.h)
endif()

if(WIN32)
    set(RFB_INCLUDE_DIR ${RFB_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/win
        )
    set(RFB_SOURCES ${RFB_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/WinPasswdValidator.cxx)
    set(RFB_HEADS ${RFB_HEADS}
        ${CMAKE_CURRENT_SOURCE_DIR}/WinPasswdValidator.h)
endif(WIN32)

set(RFB_LIBRARIES ${JPEG_LIBRARIES} ${PIXMAN_LIBRARY})

if(UNIX AND NOT APPLE)
    set(RFB_SOURCES ${RFB_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/UnixPasswordValidator.cxx)
    set(RFB_HEADS ${RFB_HEADS}
        ${CMAKE_CURRENT_SOURCE_DIR}/UnixPasswordValidator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pam.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pam.h)
    set(RFB_LIBRARIES ${RFB_LIBRARIES} ${PAM_LIBS})
endif()

if(GNUTLS_FOUND)
    set(RFB_SOURCES
        ${RFB_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityTLS.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityTLS.cxx
        )
    set(RFB_HEADS ${RFB_HEADS}
        ${CMAKE_CURRENT_SOURCE_DIR}/CSecurityTLS.h
        ${CMAKE_CURRENT_SOURCE_DIR}/SSecurityTLS.h
        )
    set(RFB_LIBRARIES
        ${RFB_LIBRARIES}
        ${GNUTLS_LIBRARIES}
        )
endif()

set(TIGERVNC_INCLUDE_DIR ${TIGERVNC_INCLUDE_DIR}
    ${RFB_INCLUDE_DIR}
    PARENT_SCOPE)

set(TIGERVNC_SOURCES ${TIGERVNC_SOURCES}
    ${RFB_SOURCES}
    PARENT_SCOPE)

set(TIGERVNC_HEADS ${TIGERVNC_HEADS}
    ${RFB_HEADS}
    PARENT_SCOPE)

set(TIGERVNC_LIBS ${TIGERVNC_LIBS}
    ${RFB_LIBRARIES}
    PARENT_SCOPE)

INSTALL(FILES ${RFB_HEADS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/rfb
    )
