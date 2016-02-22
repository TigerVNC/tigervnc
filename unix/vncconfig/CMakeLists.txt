include_directories(${X11_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/unix/tx)

include(GNUInstallDirs)

add_library(Xvnc SHARED
  vncExt.c)

set_target_properties(Xvnc
  PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

add_executable(vncconfig 
  buildtime.c
  vncconfig.cxx
  QueryConnectDialog.cxx)

target_link_libraries(vncconfig tx rfb network rdr Xvnc ${X11_LIBRARIES})

install(TARGETS vncconfig DESTINATION ${BIN_DIR})
install(TARGETS Xvnc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RENAME libXvnc.so)
install(FILES vncconfig.man DESTINATION ${MAN_DIR}/man1 RENAME vncconfig.1)
install(FILES vncExt.h DESTINATION ${X11_INCLUDE_DIR}/X11/extensions RENAME Xvnc.h)
