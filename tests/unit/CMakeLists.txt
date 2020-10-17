include_directories(${CMAKE_SOURCE_DIR}/common)

add_executable(conv conv.cxx)
target_link_libraries(conv tigervnc)

add_executable(convertlf convertlf.cxx)
target_link_libraries(convertlf tigervnc)

add_executable(hostport hostport.cxx)
target_link_libraries(hostport tigervnc)

add_executable(pixelformat pixelformat.cxx)
target_link_libraries(pixelformat tigervnc)

add_executable(unicode unicode.cxx)
target_link_libraries(unicode tigervnc)

if(BUILD_VIEWER AND FLTK_FOUND)
	include_directories(${CMAKE_SOURCE_DIR}/vncviewer)
	add_executable(gesturehandler gesturehandler.cxx ../../vncviewer/GestureHandler.cxx)
	target_link_libraries(gesturehandler tigervnc)

	add_executable(emulatemb emulatemb.cxx ../../vncviewer/EmulateMB.cxx)
	target_link_libraries(emulatemb tigervnc  ${GETTEXT_LIBRARIES})
endif()