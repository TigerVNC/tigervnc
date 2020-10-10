project(rfb_win32)

set(HEADER_FILES
    AboutDialog.h
    keymap.h
    SecurityPage.h
    BitmapInfo.h
    LaunchProcess.h
    Service.h
    CleanDesktop.h
    ListViewControl.h
    SInput.h
    Clipboard.h
    LocalMem.h
    SocketManager.h
    CompatibleBitmap.h
    ModuleFileName.h
    TCharArray.h
    ComputerName.h
    MonitorInfo.h
    TrayIcon.h
    CurrentUser.h
    MsgBox.h
    TsSessions.h
    DeviceContext.h
    MsgWindow.h
    Win32Util.h
    DeviceFrameBuffer.h
    RegConfig.h
    WMCursor.h
    Dialog.h
    Registry.h
    WMHooks.h
    DIBSectionBuffer.h
    resource.h
    WMNotifier.h
    EventManager.h
    SDisplay.h
    WMPoller.h
    Handle.h
    SDisplayCorePolling.h
    WMShatter.h
    IconInfo.h
    SDisplayCoreWMHooks.h
    WMWindowCopyRect.h
    IntervalTimer.h
    Security.h
    )

set(RFB_WIN32_SOURCES
    AboutDialog.cxx
    Clipboard.cxx
    CurrentUser.cxx
    DeviceContext.cxx
    DeviceFrameBuffer.cxx
    Dialog.cxx
    DIBSectionBuffer.cxx
    EventManager.cxx
    LaunchProcess.cxx
    ListViewControl.cxx
    MonitorInfo.cxx
    MsgWindow.cxx
    RegConfig.cxx
    Registry.cxx
    SecurityPage.cxx
    SDisplayCorePolling.cxx
    SDisplayCoreWMHooks.cxx
    SDisplay.cxx
    Security.cxx
    Service.cxx
    SInput.cxx
    SocketManager.cxx
    TCharArray.cxx
    TsSessions.cxx
    Win32Util.cxx
    WMCursor.cxx
    WMHooks.cxx
    WMNotifier.cxx
    WMPoller.cxx
    WMShatter.cxx
    WMWindowCopyRect.cxx
    CleanDesktop.cxx)

string(TOLOWER ${PROJECT_NAME} LOWER_PROJECT_NAME)
LIST(APPEND HEADER_FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/${LOWER_PROJECT_NAME}_export.h)

add_library(${PROJECT_NAME} ${RFB_WIN32_SOURCES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC rfb
    user32.lib comctl32.lib wtsapi32.lib version.lib)

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
