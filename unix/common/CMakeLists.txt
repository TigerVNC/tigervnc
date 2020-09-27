project(unixcommon)

add_library(${PROJECT_NAME} randr.cxx)

target_link_libraries(${PROJECT_NAME} PUBLIC rfb)

if(NOT WIN32)
    target_compile_options(${PROJECT_NAME} PUBLIC -fPIC)
endif()

# See: http://www.it1352.com/478094.html
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/unix/common>)

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
set(INSTALLHEADER_FILES ${INSTALLHEADER_FILES} 
    ${CMAKE_CURRENT_BINARY_DIR}/${LOWER_PROJECT_NAME}_export.h)
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
