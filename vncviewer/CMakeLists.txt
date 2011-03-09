include_directories(${FLTK_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)

add_executable(vncviewer 
  CConn.cxx
  DesktopWindow.cxx
  UserDialog.cxx
  parameters.cxx
  keysym2ucs.c
  vncviewer.cxx)

target_link_libraries(vncviewer rfb network rdr os Xregion ${FLTK_LIBRARIES})
