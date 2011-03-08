include_directories(${X11_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/unix/tx)
include_directories(${CMAKE_SOURCE_DIR}/intl)

add_executable(vncviewer 
  buildtime.c
  CConn.cxx
  DesktopWindow.cxx
  vncviewer.cxx)

target_link_libraries(vncviewer tx rfb network rdr os ${X11_LIBRARIES})
