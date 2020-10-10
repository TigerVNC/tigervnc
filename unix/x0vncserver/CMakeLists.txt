project(x0vncserver)

add_executable(${PROJECT_NAME}
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

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/unix>)

target_link_libraries(${PROJECT_NAME} PRIVATE tx unixcommon)

if(X11_FOUND AND X11_XTest_LIB)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_XTEST)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_XTest_LIB})
else()
  message(WARNING "No XTest extension. x0vncserver will be view-only.")
endif()

if(X11_FOUND AND X11_Xdamage_LIB)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_XDAMAGE)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_Xdamage_LIB})
else()
  message(WARNING "No DAMAGE extension.  x0vncserver will have to use the slower polling method.")
endif()

if(X11_FOUND AND X11_Xfixes_LIB)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_XFIXES)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_Xfixes_LIB})
else()
  message(WARNING "No XFIXES extension. x0vncserver will not be able to show cursors.")
endif()

if(X11_FOUND AND X11_Xrandr_LIB)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_XRANDR)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_Xrandr_LIB})
else()
  message(WARNING "No Xrandr extension. x0vncserver will not be able to resize session.")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        COMPONENT Runtime)
install(FILES x0vncserver.man
    DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME x0vncserver.1
        COMPONENT Runtime)
