set(ICON_FILES
  icons/tigervnc_16.png
  icons/tigervnc_22.png
  icons/tigervnc_24.png
  icons/tigervnc_32.png
  icons/tigervnc_48.png)

find_program(CONVERT_EXECUTABLE convert)

if(CONVERT_EXECUTABLE)
  add_custom_target(icons-target ALL DEPENDS ${ICON_FILES})

  if(UNIX AND NOT APPLE)
    foreach(SIZE 16 22 24 32 48)
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/icons/tigervnc_${SIZE}.png
        DESTINATION ${DATA_DIR}/icons/hicolor/${SIZE}x${SIZE}/apps
        RENAME tigervnc.png)
    endforeach()
    install(FILES icons/tigervnc.svg
      DESTINATION ${DATA_DIR}/icons/hicolor/scalable/apps)
  endif()
endif()

add_custom_command(OUTPUT icons COMMAND mkdir icons)

add_custom_command(OUTPUT icons/tigervnc_16.png
  COMMAND ${CONVERT_EXECUTABLE} -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_16.svg icons/tigervnc_16.png
  DEPENDS icons ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_16.svg
  COMMENT "Generating icons/tigervnc_16.png")

add_custom_command(OUTPUT icons/tigervnc_22.png
  COMMAND ${CONVERT_EXECUTABLE} -size 22x22 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg -composite icons/tigervnc_22.png
  DEPENDS icons ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg
  COMMENT "Generating icons/tigervnc_22.png")

add_custom_command(OUTPUT icons/tigervnc_24.png
  COMMAND ${CONVERT_EXECUTABLE} -size 24x24 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg -composite icons/tigervnc_24.png
  DEPENDS icons ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_20.svg
  COMMENT "Generating icons/tigervnc_24.png")

add_custom_command(OUTPUT icons/tigervnc_32.png
  COMMAND ${CONVERT_EXECUTABLE} -size 32x32 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_28.svg -composite icons/tigervnc_32.png
  DEPENDS icons ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_28.svg
  COMMENT "Generating icons/tigervnc_32.png")

add_custom_command(OUTPUT icons/tigervnc_48.png
  COMMAND ${CONVERT_EXECUTABLE} -size 48x48 xc:none -gravity center -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_42.svg -composite icons/tigervnc_48.png
  DEPENDS icons ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_42.svg
  COMMENT "Generating icons/tigervnc_48.png")
