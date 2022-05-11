set(ICON_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_16.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_22.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_24.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_32.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_48.png)

find_program(CONVERT_EXECUTABLE convert)

if(CONVERT_EXECUTABLE)
  add_custom_target(icons DEPENDS ${ICON_FILES})
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_16.png
  COMMAND ${CONVERT_EXECUTABLE} -depth 8 -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_16.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_16.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_16.svg
  COMMENT "Generating icons/tigervnc_16.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_22.png
  COMMAND ${CONVERT_EXECUTABLE} -depth 8 -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_22.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_22.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_22.svg
  COMMENT "Generating icons/tigervnc_22.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_24.png
  COMMAND ${CONVERT_EXECUTABLE} -depth 8 -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_24.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_24.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_24.svg
  COMMENT "Generating icons/tigervnc_24.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_32.png
  COMMAND ${CONVERT_EXECUTABLE} -depth 8 -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_32.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_32.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_32.svg
  COMMENT "Generating icons/tigervnc_32.png")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_48.png
  COMMAND ${CONVERT_EXECUTABLE} -depth 8 -background transparent
    ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_48.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_48.png
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_48.svg
  COMMENT "Generating icons/tigervnc_48.png")
