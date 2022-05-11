set(LINUX_ICON_SIZES 16 22 24 32 48 64 128)
set(WINDOWS_ICON_SIZES 16 24 32 48 256)
set(MACOS_ICON_SIZES 16 32 128 256 512)

find_program(CONVERT_EXECUTABLE convert)
find_program(PNG2ICNS_EXECUTABLE png2icns)

if(CONVERT_EXECUTABLE)
  set(ICON_FILES icons/tigervnc.ico)
  foreach(SIZE ${LINUX_ICON_SIZES})
    list(APPEND ICON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png)
  endforeach()
  if(PNG2ICNS_EXECUTABLE)
    list(APPEND ICON_FILES icons/tigervnc.icns)
  endif()
  add_custom_target(icons DEPENDS ${ICON_FILES})

  foreach(SIZE ${LINUX_ICON_SIZES} ${WINDOWS_ICON_SIZES} ${MACOS_ICON_SIZES})
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

  foreach(SIZE ${WINDOWS_ICON_SIZES})
    if(${SIZE} LESS_EQUAL 32)
      list(APPEND ICO_LEGACY \\\( ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png -depth 4 -colors 16 \\\))
      list(APPEND ICO_LEGACY \\\( ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png -depth 8 -colors 256 \\\))
    endif()
    list(APPEND ICO_NORMAL \\\( ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png \\\))
    list(APPEND ICO_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png)
  endforeach()
  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc.ico
    COMMAND ${CONVERT_EXECUTABLE} ${ICO_LEGACY} ${ICO_NORMAL}
      ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc.ico
    DEPENDS ${ICO_DEPENDS}
    COMMENT "Generating icons/tigervnc.ico")

  if(PNG2ICNS_EXECUTABLE)
    # FIXME: Should be a drop shadow here as well
    foreach(SIZE ${MACOS_ICON_SIZES})
      list(APPEND ICNS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc_${SIZE}.png)
    endforeach()
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc.icns
      COMMAND ${PNG2ICNS_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/icons/tigervnc.icns ${ICNS_FILES}
      DEPENDS ${ICNS_FILES}
      COMMENT "Generating icons/tigervnc.icns")
  endif()
endif()
