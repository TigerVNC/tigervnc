# Gettext support - mostly borrowed from the Licq project

set(po_FILES
    bg da de el eo es fi fr fur it nl pl pt_BR ru sk sr sv tr uk vi zh_CN
)

if (NOT GETTEXT_MSGMERGE_EXECUTABLE AND NOT GETTEXT_MSGFMT_EXECUTABLE)
  message(FATAL_ERROR "Gettext message catalog tools NOT found")
endif (NOT GETTEXT_MSGMERGE_EXECUTABLE AND NOT GETTEXT_MSGFMT_EXECUTABLE)

find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
if (GETTEXT_XGETTEXT_EXECUTABLE)
  # Get list of all source files
  file(GLOB_RECURSE po_source
    RELATIVE ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/vncviewer/*.h
    ${PROJECT_SOURCE_DIR}/vncviewer/*.cxx
  )

  add_custom_target(translations_update
    ${GETTEXT_XGETTEXT_EXECUTABLE}
      "--directory=${PROJECT_SOURCE_DIR}"
      "--output=${CMAKE_CURRENT_SOURCE_DIR}/tigervnc.pot"
      --default-domain=tigervnc
      --keyword=_
      --keyword=p_:1c,2
      --keyword=N_
      "--copyright-holder=TigerVNC Team and many others \(see README.txt\)"
      --msgid-bugs-address=tigervnc-devel@googlegroups.com
      --sort-by-file
      --add-location
      --add-comments=TRANSLATORS
      ${po_source}
    COMMENT "Updating tigervnc.pot"
    VERBATIM
  )
endif (GETTEXT_XGETTEXT_EXECUTABLE)

foreach(lang ${po_FILES})
  set(po  "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po")
  set(mo "${CMAKE_CURRENT_BINARY_DIR}/${lang}.mo")

  # Add command to build X.mo from X.po
  add_custom_command(OUTPUT ${mo}
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${mo} ${po}
    DEPENDS ${po}
  )

  install(FILES ${mo}
    DESTINATION "${LOCALE_DIR}/${lang}/LC_MESSAGES"
    RENAME tigervnc.mo
  )

  set(moFiles ${moFiles} ${mo})
endforeach(lang)

add_custom_target(translations ALL DEPENDS ${moFiles})
