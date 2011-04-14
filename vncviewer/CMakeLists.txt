include_directories(${FLTK_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
set(VNCVIEWER_SOURCES
  CConn.cxx
  DesktopWindow.cxx
  UserDialog.cxx
  parameters.cxx
  keysym2ucs.c
  vncviewer.cxx)

if(APPLE)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} osx_utils.m)
endif()

add_executable(vncviewer ${VNCVIEWER_SOURCES})

target_link_libraries(vncviewer rfb network rdr os Xregion ${FLTK_LIBRARIES})
