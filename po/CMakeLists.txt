# Gettext support - mostly borrowed from the Licq project

file(STRINGS LINGUAS po_FILES)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS LINGUAS)

if (NOT GETTEXT_MSGMERGE_EXECUTABLE AND NOT GETTEXT_MSGFMT_EXECUTABLE)
  message(FATAL_ERROR "Gettext message catalog tools NOT found")
endif (NOT GETTEXT_MSGMERGE_EXECUTABLE AND NOT GETTEXT_MSGFMT_EXECUTABLE)

find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
if (GETTEXT_XGETTEXT_EXECUTABLE)
  # Get list of all source files
  file(GLOB po_source
    RELATIVE ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/common/core/*.h
    ${PROJECT_SOURCE_DIR}/common/core/*.cxx
    ${PROJECT_SOURCE_DIR}/common/network/*.h
    ${PROJECT_SOURCE_DIR}/common/network/*.cxx
    ${PROJECT_SOURCE_DIR}/common/rdr/*.h
    ${PROJECT_SOURCE_DIR}/common/rdr/*.cxx
    ${PROJECT_SOURCE_DIR}/common/rfb/*.h
    ${PROJECT_SOURCE_DIR}/common/rfb/*.cxx
    ${PROJECT_SOURCE_DIR}/unix/common/*.h
    ${PROJECT_SOURCE_DIR}/unix/common/*.cxx
    ${PROJECT_SOURCE_DIR}/unix/tx/*.h
    ${PROJECT_SOURCE_DIR}/unix/tx/*.cxx
    ${PROJECT_SOURCE_DIR}/unix/vncconfig/*.h
    ${PROJECT_SOURCE_DIR}/unix/vncconfig/*.cxx
    ${PROJECT_SOURCE_DIR}/unix/vncpasswd/*.h
    ${PROJECT_SOURCE_DIR}/unix/vncpasswd/*.cxx
    ${PROJECT_SOURCE_DIR}/unix/vncserver/*.h
    ${PROJECT_SOURCE_DIR}/unix/vncserver/*.c
    ${PROJECT_SOURCE_DIR}/unix/x0vncserver/*.h
    ${PROJECT_SOURCE_DIR}/unix/x0vncserver/*.cxx
    ${PROJECT_SOURCE_DIR}/unix/xserver/hw/vnc/*.h
    ${PROJECT_SOURCE_DIR}/unix/xserver/hw/vnc/*.c
    ${PROJECT_SOURCE_DIR}/unix/xserver/hw/vnc/*.cc
    ${PROJECT_SOURCE_DIR}/vncviewer/*.h
    ${PROJECT_SOURCE_DIR}/vncviewer/*.c
    ${PROJECT_SOURCE_DIR}/vncviewer/*.cxx
    ${PROJECT_SOURCE_DIR}/vncviewer/*.m
    ${PROJECT_SOURCE_DIR}/vncviewer/*.mm
    ${PROJECT_SOURCE_DIR}/vncviewer/*.desktop.in.in
    ${PROJECT_SOURCE_DIR}/vncviewer/*.metainfo.xml.in
    ${PROJECT_SOURCE_DIR}/win/rfb_win32/*.h
    ${PROJECT_SOURCE_DIR}/win/rfb_win32/*.cxx
    ${PROJECT_SOURCE_DIR}/win/vncconfig/*.h
    ${PROJECT_SOURCE_DIR}/win/vncconfig/*.cxx
    ${PROJECT_SOURCE_DIR}/win/winvnc/*.h
    ${PROJECT_SOURCE_DIR}/win/winvnc/*.cxx
    ${PROJECT_SOURCE_DIR}/win/wm_hooks/*.h
    ${PROJECT_SOURCE_DIR}/win/wm_hooks/*.cxx
  )

  add_custom_target(translations_update
    ${GETTEXT_XGETTEXT_EXECUTABLE}
      "--directory=${PROJECT_SOURCE_DIR}"
      "--output=${CMAKE_CURRENT_SOURCE_DIR}/tigervnc.pot"
      --default-domain=tigervnc
      --from-code=UTF-8
      --keyword=_
      --keyword=C_:1c,2
      --keyword=N_
      --keyword=NC_:1c,2
      "--copyright-holder=TigerVNC team and many others \(see README.rst\)"
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
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --check --check-accelerators -o ${mo} ${po}
    DEPENDS ${po}
  )

  install(FILES ${mo}
    DESTINATION "${CMAKE_INSTALL_FULL_LOCALEDIR}/${lang}/LC_MESSAGES"
    RENAME tigervnc.mo
  )

  set(moFiles ${moFiles} ${mo})
endforeach(lang)

add_custom_target(translations ALL DEPENDS ${moFiles})
