# Author: Kang Lin <kl222@126.com>

project(rdr)

# Check for zlib
find_package(ZLIB REQUIRED)
set(RDR_HEADFILES ${ZLIB_INCLUDE_DIRS})
set(RDR_LIBRARIES ${ZLIB_LIBRARIES} network)

set(HEADER_FILES
    Exception.h
    HexInStream.h
    MemOutStream.h
    ZlibOutStream.h
    FdInStream.h
    HexOutStream.h
    OutStream.h
    FdOutStream.h
    InStream.h
    RandomStream.h
    types.h
    FileInStream.h
    MemInStream.h
    ZlibInStream.h    
    )

SET(RDR_FILES
    Exception.cxx
    FdInStream.cxx
    FdOutStream.cxx
    FileInStream.cxx
    HexInStream.cxx
    HexOutStream.cxx
    InStream.cxx
    RandomStream.cxx
    ZlibInStream.cxx
    ZlibOutStream.cxx
    )
if(GNUTLS_FOUND)
    LIST(APPEND RDR_LIBRARIES ${GNUTLS_LIBRARIES})
    LIST(APPEND RDR_DEFINITIONS HAVE_GNUTLS ${GNUTLS_DEFINITIONS})
    LIST(APPEND RDR_HEADFILES ${GNUTLS_INCLUDE_DIR})
    LIST(APPEND HEADER_FILES
        TLSInStream.h
        TLSOutStream.h    
        TLSException.h
        )
    LIST(APPEND RDR_FILES
        TLSException.cxx
        TLSInStream.cxx
        TLSOutStream.cxx
        )
endif()

string(TOLOWER ${PROJECT_NAME} LOWER_PROJECT_NAME)
LIST(APPEND HEADER_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${LOWER_PROJECT_NAME}_export.h)

add_library(${PROJECT_NAME} ${HEADER_FILES} ${RDR_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${RDR_HEADFILES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${RDR_DEFINITIONS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${RDR_LIBRARIES})

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
