if(CMAKE_CL_64)
  set(INST_NAME ${CMAKE_PROJECT_NAME}64-${VERSION})
  set(INST_DEFS -DWIN64)
else()
  set(INST_NAME ${CMAKE_PROJECT_NAME}-${VERSION})
endif()

if(MSVC_IDE)
  set(INSTALLERDIR "$(OutDir)")
  set(BUILDDIRDEF "-DBUILD_DIR=${INSTALLERDIR}\\")
else()
  set(INSTALLERDIR .)
  set(BUILDDIRDEF "-DBUILD_DIR=")
endif()

set(INST_DEPS vncviewer)

if(BUILD_WINVNC)
  set(INST_DEFS ${INST_DEFS} -DBUILD_WINVNC)
  set(INST_DEPS ${INST_DEPS} winvnc4 wm_hooks vncconfig)
endif()

configure_file(tigervnc.iss.in tigervnc.iss)

add_custom_target(installer
  iscc -o${INSTALLERDIR} ${INST_DEFS} ${BUILDDIRDEF} -F${INST_NAME} tigervnc.iss
  DEPENDS ${INST_DEPS}
  SOURCES tigervnc.iss)

install(FILES ${CMAKE_SOURCE_DIR}/win/README_BINARY.txt
  ${CMAKE_SOURCE_DIR}/LICENCE.txt DESTINATION .)
