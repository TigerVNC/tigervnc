set(ICON_SIZES 16 22 24 32 48)

find_program(CONVERT_EXECUTABLE convert)

if(CONVERT_EXECUTABLE)
  foreach(SIZE ${ICON_SIZES})
    list(APPEND ICON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png)
  endforeach()
  add_custom_target(icons DEPENDS ${ICON_FILES})

  foreach(SIZE ${ICON_SIZES})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png
      COMMAND ${CONVERT_EXECUTABLE} -depth 8 -background transparent
        ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_${SIZE}.svg
        ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_${SIZE}.svg
      COMMENT "Generating icons/tigervnc_${SIZE}.png")
  endforeach()
endif()
