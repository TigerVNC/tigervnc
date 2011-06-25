include_directories(${FLTK_INCLUDE_DIR})
include_directories(${GETTEXT_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
set(VNCVIEWER_SOURCES
  CConn.cxx
  DesktopWindow.cxx
  UserDialog.cxx
  ServerDialog.cxx
  OptionsDialog.cxx
  Viewport.cxx
  parameters.cxx
  keysym2ucs.c
  vncviewer.cxx)

if(WIN32)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} win32.c)
endif()

if(APPLE)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} cocoa.mm)
endif()

if(WIN32)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} Win32PixelBuffer.cxx)
elseif(APPLE)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} OSXPixelBuffer.cxx)
else()
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} X11PixelBuffer.cxx)
endif()

add_executable(vncviewer ${VNCVIEWER_SOURCES})

target_link_libraries(vncviewer rfb network rdr os Xregion ${FLTK_LIBRARIES} ${GETTEXT_LIBRARIES})

install(TARGETS vncviewer DESTINATION bin)
if(UNIX)
  install(FILES vncviewer.man DESTINATION man/man1 RENAME vncviewer.1)
endif()
