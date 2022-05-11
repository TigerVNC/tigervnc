set(ICON_SIZES 16 22 24 32 48 64 128)

find_program(CONVERT_EXECUTABLE convert)

if(CONVERT_EXECUTABLE)
  foreach(SIZE ${ICON_SIZES})
    list(APPEND ICON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png)
  endforeach()
  add_custom_target(icons DEPENDS ${ICON_FILES})

  foreach(SIZE ${ICON_SIZES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_${SIZE}.svg)
      set(ICON_SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_${SIZE}.svg)
    else()
      set(ICON_SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tigervnc_48.svg)
    endif()
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png
      COMMAND ${CONVERT_EXECUTABLE} -depth 8 -background transparent
        -size ${SIZE}x${SIZE} ${ICON_SRC_FILE}
        ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png
      DEPENDS ${ICON_SRC_FILE}
      COMMENT "Generating icons/tigervnc_${SIZE}.png")
  endforeach()
endif()
