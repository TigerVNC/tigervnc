include_directories(${GETTEXT_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(test_util STATIC util.cxx)
target_link_libraries(test_util ${H264_LIBRARIES})

add_executable(convperf convperf.cxx)
target_link_libraries(convperf test_util rfb)

add_executable(decperf decperf.cxx)
target_link_libraries(decperf test_util rfb)

add_executable(encperf encperf.cxx)
target_link_libraries(encperf test_util rfb)

if (BUILD_VIEWER)
  include_directories(${FLTK_INCLUDE_DIR})
  set(FBPERF_SOURCES
    fbperf.cxx
    ${CMAKE_SOURCE_DIR}/vncviewer/PlatformPixelBuffer.cxx
    ${CMAKE_SOURCE_DIR}/vncviewer/Surface.cxx)
  if(WIN32)
    set(FBPERF_SOURCES ${FBPERF_SOURCES} ${CMAKE_SOURCE_DIR}/vncviewer/Surface_Win32.cxx)
  elseif(APPLE)
    set(FBPERF_SOURCES
        ${FBPERF_SOURCES} ${CMAKE_SOURCE_DIR}/vncviewer/Surface_OSX.cxx
        ${FBPERF_SOURCES} ${CMAKE_SOURCE_DIR}/vncviewer/keysym2ucs.c
        ${FBPERF_SOURCES} ${CMAKE_SOURCE_DIR}/vncviewer/cocoa.mm)
  else()
    set(FBPERF_SOURCES ${FBPERF_SOURCES} ${CMAKE_SOURCE_DIR}/vncviewer/Surface_X11.cxx)
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
endif()
