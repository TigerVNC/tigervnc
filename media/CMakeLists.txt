set(ICON_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_16.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_22.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_24.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_32.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_48.png)

find_program(CONVERT_EXECUTABLE convert)

if(CONVERT_EXECUTABLE)
  add_custom_target(icons DEPENDS ${ICON_FILES})

  if(UNIX AND NOT APPLE)
    foreach(SIZE 16 22 24 32 48)
      install(FILES icons/tigervnc_${SIZE}.png
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/${SIZE}x${SIZE}/apps
        RENAME tigervnc.png)
    endforeach()
    install(FILES icons/tigervnc.svg
      DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps)
  endif()
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_16.png
  COMMAND ${CONVERT_EXECUTABLE} -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_16.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_16.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_16.svg
  COMMENT "Generating icons/tigervnc_16.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_22.png
  COMMAND ${CONVERT_EXECUTABLE} -density 72
    -size 22x22 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg -composite
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_22.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg
  COMMENT "Generating icons/tigervnc_22.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_24.png
  COMMAND ${CONVERT_EXECUTABLE} -density 72
    -size 24x24 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg -composite
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_24.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg
  COMMENT "Generating icons/tigervnc_24.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_32.png
  COMMAND ${CONVERT_EXECUTABLE} -density 72
    -size 32x32 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_28.svg -composite
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_32.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_28.svg
  COMMENT "Generating icons/tigervnc_32.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_48.png
  COMMAND ${CONVERT_EXECUTABLE} -density 72
    -size 48x48 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_42.svg -composite
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_48.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_42.svg
  COMMENT "Generating icons/tigervnc_48.png")
