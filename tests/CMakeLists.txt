include_directories(${FLTK_INCLUDE_DIR})
include_directories(${GETTEXT_INCLUDE_DIR})

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
  ../vncviewer/PlatformPixelBuffer.cxx
  ../vncviewer/Surface.cxx)
if(WIN32)
  set(FBPERF_SOURCES ${FBPERF_SOURCES} ../vncviewer/Surface_Win32.cxx)
elseif(APPLE)
  set(FBPERF_SOURCES
      ${FBPERF_SOURCES} ../vncviewer/Surface_OSX.cxx
      ${FBPERF_SOURCES} ../vncviewer/keysym2ucs.c
      ${FBPERF_SOURCES} ../vncviewer/cocoa.mm)
else()
  set(FBPERF_SOURCES ${FBPERF_SOURCES} ../vncviewer/Surface_X11.cxx)
endif()
add_executable(fbperf ${FBPERF_SOURCES})
target_link_libraries(fbperf test_util rfb ${FLTK_LIBRARIES} ${GETTEXT_LIBRARIES})
if(WIN32)
  target_link_libraries(fbperf msimg32)
endif()
if(APPLE)
  target_link_libraries(fbperf "-framework Cocoa")
  target_link_libraries(fbperf "-framework Carbon")
  target_link_libraries(fbperf "-framework IOKit")
endif()
