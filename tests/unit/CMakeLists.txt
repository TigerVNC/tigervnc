include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/vncviewer)
include_directories(${GETTEXT_INCLUDE_DIR})

add_executable(conv conv.cxx)
target_link_libraries(conv rfb)

add_executable(convertlf convertlf.cxx)
target_link_libraries(convertlf rfb)

add_executable(gesturehandler gesturehandler.cxx ../../vncviewer/GestureHandler.cxx)
target_link_libraries(gesturehandler rfb)

add_executable(hostport hostport.cxx)
target_link_libraries(hostport rfb)

add_executable(pixelformat pixelformat.cxx)
target_link_libraries(pixelformat rfb)

add_executable(unicode unicode.cxx)
target_link_libraries(unicode rfb)

add_executable(emulatemb emulatemb.cxx ../../vncviewer/EmulateMB.cxx)
target_link_libraries(emulatemb rfb  ${GETTEXT_LIBRARIES})
