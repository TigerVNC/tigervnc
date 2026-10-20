if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_library(test_util STATIC util.cxx)

add_executable(convperf convperf.cxx)
target_link_libraries(convperf PRIVATE test_util rfb)

add_executable(decperf decperf.cxx)
target_link_libraries(decperf PRIVATE test_util rdr rfb rfbclient)

add_executable(encperf encperf.cxx)
target_link_libraries(encperf PRIVATE test_util core rdr rfb rfbclient rfbserver)

if (BUILD_VIEWER)
  add_executable(fbperf
    fbperf.cxx
    ${CMAKE_SOURCE_DIR}/vncviewer/PlatformPixelBuffer.cxx
    ${CMAKE_SOURCE_DIR}/vncviewer/Surface.cxx)
  if(WIN32)
    target_sources(fbperf PRIVATE ${CMAKE_SOURCE_DIR}/vncviewer/Surface_Win32.cxx)
  elseif(APPLE)
    target_sources(fbperf PRIVATE
      ${CMAKE_SOURCE_DIR}/vncviewer/Surface_OSX.cxx
      ${CMAKE_SOURCE_DIR}/vncviewer/keysym2ucs.c
      ${CMAKE_SOURCE_DIR}/vncviewer/cocoa.mm)
  else()
    target_sources(fbperf PRIVATE ${CMAKE_SOURCE_DIR}/vncviewer/Surface_X11.cxx)
  endif()
  target_link_libraries(fbperf PRIVATE test_util core rfb)
  target_include_directories(fbperf SYSTEM PUBLIC ${FLTK_INCLUDE_DIR})
  target_link_libraries(fbperf PRIVATE ${FLTK_LIBRARIES})
  if(WIN32)
    target_link_libraries(fbperf PRIVATE msimg32)
  endif()
  if(APPLE)
    target_link_libraries(fbperf PRIVATE "-framework Cocoa")
    target_link_libraries(fbperf PRIVATE "-framework Carbon")
    target_link_libraries(fbperf PRIVATE "-framework IOKit")
  endif()
endif()
