project(winvnc4)

set(WINVNC_HEADS
	AddNewClientDialog.h
	ManagedListener.h
	STrayIcon.h
	ControlPanel.h
	QueryConnectDialog.h
	VNCServerService.h
	ListConnInfo.h
	resource.h
	VNCServerWin32.h
)

set(WINVNC_SOURCE
	buildTime.cxx
  ControlPanel.cxx
  ManagedListener.cxx
  QueryConnectDialog.cxx
  STrayIcon.cxx
  VNCServerService.cxx
  VNCServerWin32.cxx
  winvnc.cxx
)

add_executable(${PROJECT_NAME} WIN32
	${WINVNC_SOURCE} ${WINVNC_HEADS} winvnc.rc)

target_link_libraries(${PROJECT_NAME} PRIVATE
    rfb_win32 rfb)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/win)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    VERSION ${VERSION})

INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Runtime
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT Runtime
    )
