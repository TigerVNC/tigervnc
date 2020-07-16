# This file is included from the top-level CMakeLists.txt.  We just store it
# here to avoid cluttering up that file.


#
# Windows installer (Inno Setup)
#

if(WIN32)

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(INST_NAME ${CMAKE_PROJECT_NAME}64-${VERSION})
  set(INST_DEFS -DWIN64)
else()
  set(INST_NAME ${CMAKE_PROJECT_NAME}-${VERSION})
endif()

set(INST_DEPS vncviewer)

if(BUILD_WINVNC)
  set(INST_DEFS ${INST_DEFS} -DBUILD_WINVNC)
  if(BUILD_JAVA)
    set(INST_DEPS ${INST_DEPS} java)
  endif()
  set(INST_DEPS ${INST_DEPS} winvnc4 wm_hooks vncconfig)
endif()

configure_file(tigervnc.iss.in tigervnc.iss)

add_custom_target(installer
  iscc -o. ${INST_DEFS} -F${INST_NAME} tigervnc.iss
  DEPENDS ${INST_DEPS}
  SOURCES tigervnc.iss)

endif() # WIN32


#
# Mac DMG
#

if(APPLE)

set(DEFAULT_OSX_X86_BUILD ${CMAKE_SOURCE_DIR}/osxx86)
set(OSX_X86_BUILD ${DEFAULT_OSX_X86_BUILD} CACHE PATH
  "Directory containing 32-bit OS X build to include in universal binaries (default: ${DEFAULT_OSX_X86_BUILD})")

configure_file(makemacapp.in makemacapp)
configure_file(Info.plist.in Info.plist)

add_custom_target(dmg sh makemacapp
  DEPENDS vncviewer
  SOURCES makemacapp)

add_custom_target(udmg sh makemacapp universal
  DEPENDS vncviewer
  SOURCES makemacapp)

endif() # APPLE


#
# Binary tarball
#

if(UNIX)

configure_file(maketarball.in maketarball)

set(TARBALL_DEPENDS vncviewer vncpasswd vncconfig)
if(BUILD_JAVA)
  set(TARBALL_DEPENDS ${TARBALL_DEPENDS} java)
endif()

add_custom_target(tarball bash maketarball
  DEPENDS ${TARBALL_DEPENDS}
  SOURCES maketarball)

add_custom_target(servertarball bash maketarball server
  DEPENDS ${TARBALL_DEPENDS}
  SOURCES maketarball)

endif() #UNIX

#
# Common
#

install(FILES ${CMAKE_SOURCE_DIR}/LICENCE.TXT DESTINATION ${CMAKE_INSTALL_FULL_DOCDIR})
install(FILES ${CMAKE_SOURCE_DIR}/README.rst DESTINATION ${CMAKE_INSTALL_FULL_DOCDIR})
