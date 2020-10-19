project(tigervnc)

add_subdirectory(os)
add_subdirectory(rdr)
add_subdirectory(rfb)
add_subdirectory(network)

add_library(${PROJECT_NAME} ${TIGERVNC_SOURCES} ${TIGERVNC_HEADS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${TIGERVNC_LIBS})

# See: http://www.it1352.com/478094.html
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common>)

target_include_directories(${PROJECT_NAME} PRIVATE ${TIGERVNC_INCLUDE_DIR})

# For any convenience libraries that are linked into libvnc.so, we need to
# explicitly build their corresponding sources using PIC.  WIN32 is excluded
# because PIC code does not exist on that platform and MinGW complains if -fPIC
# is passed (additionally, libvnc is not used on Windows.)

if(NOT WIN32)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES COMPILE_FLAGS -fPIC)
endif()

#set_target_properties(${PROJECT_NAME} PROPERTIES
#    PUBLIC_HEADER "${TIGERVNC_HEADS}" # Install head files
#    )

#Install target
if(WIN32)
    INSTALL(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Config
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
        LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        )
else()
    INSTALL(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Config
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT Runtime
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        )
endif()

export(TARGETS ${PROJECT_NAME}
    APPEND FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    )
# Install cmake configure files
install(EXPORT ${PROJECT_NAME}Config
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake"
    )
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion)
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake")

if(UNIX)
    libtool_create_control_file(${PROJECT_NAME})
endif()
