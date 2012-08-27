include_directories(${FLTK_INCLUDE_DIR})
include_directories(${GETTEXT_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
set(VNCVIEWER_SOURCES
  buildTime.cxx
  menukey.cxx
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
  # Since vncviewer.rc is generated, local includes will be looking
  # in the wrong directory. We need to help it out.
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  configure_file(vncviewer.rc.in vncviewer.rc)
  set(VNCVIEWER_SOURCES
    ${VNCVIEWER_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/vncviewer.rc)
endif()

if(WIN32)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} win32.c)
elseif(APPLE)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} cocoa.mm)
endif()

if(WIN32)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} Win32PixelBuffer.cxx)
elseif(APPLE)
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} OSXPixelBuffer.cxx)
else()
  set(VNCVIEWER_SOURCES ${VNCVIEWER_SOURCES} X11PixelBuffer.cxx)
endif()

if(WIN32)
  add_executable(vncviewer WIN32 ${VNCVIEWER_SOURCES})
else()
  add_executable(vncviewer ${VNCVIEWER_SOURCES})
endif()


# XXX: Cendio hack
#target_link_libraries(vncviewer rfb network rdr os Xregion ${FLTK_LIBRARIES} ${GETTEXT_LIBRARIES})
#
## When building with GnuTLS, librdr depends on ws2_32, so in order to make
## MinGW happy, we need to put ws2_32 in librdr's target_link_libraries string,
## not here.
#if(NOT GNUTLS_FOUND AND WIN32)
#  target_link_libraries(vncviewer ws2_32)
#endif()

target_link_libraries(vncviewer -nodefaultlibs rfb network rdr os Xregion)

if(APPLE)
  target_link_libraries(vncviewer "-framework Carbon" "-framework Cocoa" "-framework ApplicationServices")
endif()

if(UNIX AND NOT APPLE)
  # Needed to load icon files
  target_link_libraries(vncviewer -Wl,-Bstatic ${FLTK_IMAGES_LIBRARY} png -Wl,-Bdynamic)
endif()

target_link_libraries(vncviewer -Wl,-Bstatic ${FLTK_BASE_LIBRARY} -Wl,-Bdynamic)

if(WIN32)
  target_link_libraries(vncviewer -Wl,-Bstatic ${GETTEXT_LIBRARIES} ${ICONV_LIBRARIES} -Wl,-Bdynamic)
elseif(APPLE)
  target_link_libraries(vncviewer -Wl,-Bstatic ${GETTEXT_LIBRARIES} -Wl,-Bdynamic ${ICONV_LIBRARIES})
else()
  if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    # XXX: Cendio hack: Add rpath to find libXfixes on Solaris without LD_LIBRARY_PATH.
    # XXX: Cendio hack: libXft is broken on at least our servers
    # XXX: Cendio hack: In theory, we must add freetype. However, in practice this does not work, since it is not found,
    # since it's in /usr/sparc-sun-solaris2.10/sys-root/usr/sfw/lib/. It works if we DONT specify it, though. 
    target_link_libraries(vncviewer ${X11_Xcursor_LIB} ${X11_Xfixes_LIB} /usr/sparc-sun-solaris2.10/sys-root/usr/X11/lib/libXinerama.so -Wl,-Bstatic Xft -Wl,-Bdynamic fontconfig Xext -R/usr/sfw/lib)
  else()
    target_link_libraries(vncviewer -Wl,-Bstatic ${X11_Xcursor_LIB} ${X11_Xfixes_LIB} Xft fontconfig expat freetype Xrender Xext Xinerama -Wl,-Bdynamic)
  endif()
  target_link_libraries(vncviewer X11 m )
endif()

target_link_libraries(vncviewer libstdc++.a gcc gcc_eh)

if(WIN32)
  target_link_libraries(vncviewer ws2_32 comctl32 mingw32 moldname mingwex msvcrt kernel32 gcc)
else()
  target_link_libraries(vncviewer c)
endif()

install(TARGETS vncviewer DESTINATION ${BIN_DIR})
if(UNIX)
  install(FILES vncviewer.man DESTINATION ${MAN_DIR}/man1 RENAME vncviewer.1)
endif()
