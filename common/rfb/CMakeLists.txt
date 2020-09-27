# Author: Kang Lin <kl222@126.com>

project(rfb)

# Check for libjpeg
find_package(JPEG REQUIRED)
SET(RFB_INCLUDE_HEADS ${JPEG_INCLUDE_DIR})
set(RFB_LIBRARIES ${JPEG_LIBRARIES})

# Check for pixman
find_package(Pixman REQUIRED)
list(APPEND RFB_INCLUDE_HEADS ${PIXMAN_INCLUDE_DIR})
list(APPEND RFB_LIBRARIES ${PIXMAN_LIBRARY})

set(HEADER_FILES
    Blacklist.h
    Congestion.h
    CConnection.h
    CMsgHandler.h
    CMsgReader.h
    CMsgWriter.h
    CSecurityPlain.h
    CSecurityStack.h
    CSecurityVeNCrypt.h
    CSecurityVncAuth.h
    ClientParams.h
    ComparingUpdateTracker.h
    Configuration.h
    CopyRectDecoder.h
    Cursor.h
    DecodeManager.h
    Decoder.h
    d3des.h
    EncodeManager.h
    Encoder.h
    HextileDecoder.h
    HextileEncoder.h
    JpegCompressor.h
    JpegDecompressor.h
    KeyRemapper.h
    LogWriter.h
    Logger.h
    Logger_file.h
    Logger_stdio.h
    Password.h
    PixelBuffer.h
    PixelFormat.h
    RREEncoder.h
    RREDecoder.h
    RawDecoder.h
    RawEncoder.h
    Region.h
    SConnection.h
    SMsgHandler.h
    SMsgReader.h
    SMsgWriter.h
    ServerCore.h
    ServerParams.h
    Security.h
    SecurityServer.h
    SecurityClient.h
    SSecurityPlain.h
    SSecurityStack.h
    SSecurityVncAuth.h
    SSecurityVeNCrypt.h
    ScaleFilters.h
    Timer.h
    TightDecoder.h
    TightEncoder.h
    TightJPEGEncoder.h
    UpdateTracker.h
    VNCSConnectionST.h
    VNCServerST.h
    ZRLEEncoder.h
    ZRLEDecoder.h
    encodings.h
    util.h
    )
set(RFB_SOURCES
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

if(UNIX)
    LIST(APPEND RFB_SOURCES Logger_syslog.cxx)
    LIST(APPEND HEADER_FILES Logger_syslog.h)
endif()

if(WIN32)
    LIST(APPEND RFB_SOURCES WinPasswdValidator.cxx)
    LIST(APPEND HEADER_FILES WinPasswdValidator.h)
    LIST(APPEND RFB_INCLUDE_HEADS ${CMAKE_SOURCE_DIR}/win)
endif(WIN32)

if(UNIX AND NOT APPLE)
    LIST(APPEND RFB_SOURCES UnixPasswordValidator.cxx pam.c)
    LIST(APPEND HEADER_FILES UnixPasswordValidator.h pam.h)
    LIST(APPEND RFB_LIBRARIES ${PAM_LIBS})
endif()

if(GNUTLS_FOUND)
    set(RFB_SOURCES
        ${RFB_SOURCES}
        CSecurityTLS.cxx
        SSecurityTLS.cxx
        )
    LIST(APPEND HEADER_FILES
        CSecurityTLS.h
        SSecurityTLS.h
        )
    list(APPEND RFB_LIBRARIES ${GNUTLS_LIBRARIES})
    list(APPEND RFB_INCLUDE_HEADS ${GNUTLS_INCLUDE_DIR})
    list(APPEND RFB_DEFINITIONS HAVE_GNUTLS ${GNUTLS_DEFINITIONS})
endif()

LIST(APPEND RFB_LIBRARIES rdr)

string(TOLOWER ${PROJECT_NAME} LOWER_PROJECT_NAME)
LIST(APPEND HEADER_FILES ${HEADER_FILES} 
    ${CMAKE_CURRENT_BINARY_DIR}/${LOWER_PROJECT_NAME}_export.h)

add_library(${PROJECT_NAME} ${RFB_SOURCES} ${HEADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${RFB_INCLUDE_HEADS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${RFB_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${RFB_DEFINITIONS})

if(UNIX)
    libtool_create_control_file(${PROJECT_NAME})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    PUBLIC_HEADER "${HEADER_FILES}" # Install head files
    VERSION ${VERSION}
    )

GENERATE_EXPORT_HEADER(${PROJECT_NAME})
string(TOLOWER ${PROJECT_NAME} LOWER_PROJECT_NAME)
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${LOWER_PROJECT_NAME}_export.h
    DESTINATION ${CMAKE_BINARY_DIR})

# Install target
if(ANDROID)
    INSTALL(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Config
        RUNTIME DESTINATION "libs/${ANDROID_ABI}"
        COMPONENT Runtime
        LIBRARY DESTINATION "libs/${ANDROID_ABI}"
        COMPONENT Runtime
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc/${PROJECT_NAME}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc
        )
else()
    if(WIN32)
        INSTALL(TARGETS ${PROJECT_NAME}
            EXPORT ${PROJECT_NAME}Config
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
            LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc/${PROJECT_NAME}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc
            )
    else()
        INSTALL(TARGETS ${PROJECT_NAME}
            EXPORT ${PROJECT_NAME}Config
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT Runtime
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc/${PROJECT_NAME}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc
            )
    endif()
endif()

# Export build cmake configure files
export(TARGETS ${PROJECT_NAME}
    APPEND FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    )

# Install cmake configure files
install(EXPORT ${PROJECT_NAME}Config
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tigervnc"
    NAMESPACE
        tigervnc::
    )
# Install cmake configure version files
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion)
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tigervnc")
