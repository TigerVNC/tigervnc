add_executable(x0vncserver
  buildtime.c
  Geometry.cxx
  Image.cxx
  PollingManager.cxx
  PollingScheduler.cxx
  TimeMillis.cxx
  qnum_to_xorgevdev.c
  qnum_to_xorgkbd.c
  x0vncserver.cxx
  XPixelBuffer.cxx
  XDesktop.cxx
  RandrGlue.c
  ../vncconfig/QueryConnectDialog.cxx
)

target_include_directories(x0vncserver SYSTEM PUBLIC ${X11_INCLUDE_DIR})
target_include_directories(x0vncserver PUBLIC ${CMAKE_SOURCE_DIR}/unix/common)
target_include_directories(x0vncserver PUBLIC ${CMAKE_SOURCE_DIR}/unix/tx)
target_include_directories(x0vncserver PUBLIC ${CMAKE_SOURCE_DIR}/unix)
target_include_directories(x0vncserver PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(x0vncserver tx rfb network rdr unixcommon)

# systemd support (socket activation)
if (LIBSYSTEMD_FOUND)
  target_link_libraries(x0vncserver ${LIBSYSTEMD_LIBRARIES})
endif()

if(X11_XTest_LIB)
  target_link_libraries(x0vncserver ${X11_XTest_LIB})
else()
  message(WARNING "No XTest extension.  x0vncserver will be view-only.")
endif()

if(X11_Xdamage_LIB)
  target_link_libraries(x0vncserver ${X11_Xdamage_LIB})
else()
  message(WARNING "No DAMAGE extension.  x0vncserver will have to use the slower polling method.")
endif()

if(X11_Xfixes_LIB)
  target_link_libraries(x0vncserver ${X11_Xfixes_LIB})
else()
  message(WARNING "No XFIXES extension.  x0vncserver will not be able to show cursors.")
endif()

if(X11_Xrandr_LIB)
  target_link_libraries(x0vncserver ${X11_Xrandr_LIB})
else()
  message(WARNING "No Xrandr extension.  x0vncserver will not be able to resize session.")
endif()

target_link_libraries(x0vncserver ${X11_LIBRARIES})

install(TARGETS x0vncserver DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES x0vncserver.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME x0vncserver.1)
