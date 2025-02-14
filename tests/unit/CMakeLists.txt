include(GoogleTest)

# Google Test requires a new C++ standard
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")

enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/vncviewer)

add_executable(configargs configargs.cxx)
target_link_libraries(configargs rfb)

add_executable(conv conv.cxx)
target_link_libraries(conv rfb)

add_executable(convertlf convertlf.cxx)
target_link_libraries(convertlf core)

add_executable(gesturehandler gesturehandler.cxx ../../vncviewer/GestureHandler.cxx)
target_link_libraries(gesturehandler core)

add_executable(hostport hostport.cxx)
target_link_libraries(hostport network)

add_executable(pixelformat pixelformat.cxx)
target_link_libraries(pixelformat rfb)

add_executable(unicode unicode.cxx)
target_link_libraries(unicode core)

add_executable(emulatemb emulatemb.cxx ../../vncviewer/EmulateMB.cxx)
target_include_directories(emulatemb SYSTEM PUBLIC ${GETTEXT_INCLUDE_DIR})
target_link_libraries(emulatemb core ${GETTEXT_LIBRARIES})
