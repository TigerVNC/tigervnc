include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(test_util STATIC util.cxx)

add_executable(convperf convperf.cxx)
target_link_libraries(convperf test_util rfb)

add_executable(conv conv.cxx)
target_link_libraries(conv rfb)

add_executable(decperf decperf.cxx)
target_link_libraries(decperf test_util rfb)

add_executable(encperf encperf.cxx)
target_link_libraries(encperf test_util rfb)

add_executable(hostport hostport.cxx)
target_link_libraries(hostport rfb)

set(FBPERF_SOURCES
  fbperf.cxx
  ../vncviewer/FLTKPixelBuffer.cxx
  ../vncviewer/PlatformPixelBuffer.cxx)
if(WIN32)
  set(FBPERF_SOURCES ${FBPERF_SOURCES} ../vncviewer/Win32PixelBuffer.cxx)
elseif(APPLE)
  set(FBPERF_SOURCES ${FBPERF_SOURCES} ../vncviewer/OSXPixelBuffer.cxx)
else()
  set(FBPERF_SOURCES ${FBPERF_SOURCES} ../vncviewer/X11PixelBuffer.cxx)
endif()
add_executable(fbperf ${FBPERF_SOURCES})
target_link_libraries(fbperf test_util rfb ${FLTK_LIBRARIES} ${GETTEXT_LIBRARIES})
if(APPLE)
  target_link_libraries(fbperf "-framework Cocoa" "-framework Carbon")
endif()
