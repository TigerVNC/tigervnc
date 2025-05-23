cmake_parse_arguments(
  GIO_FIND_ARGS
  "REQUIRED;OPTIONAL;QUIET"
  ""
  ""
  ${ARGN}
)

set(gio_find_args "")

if(GIO_FIND_ARGS_QUIET)
  list(APPEND gio_find_args QUIET)
endif()

if (GIO_FIND_ARGS_REQUIRED)
  list(APPEND gio_find_args REQUIRED)
elseif(GIO_FIND_ARGS_OPTIONAL)
  list(APPEND gio_find_args OPTIONAL)
endif()

find_package(GLib ${gio_find_args})

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(GIO gio-2.0 ${gio_find_args})
endif()


if(NOT GIO_FOUND)
  find_path(GIO_INCLUDE_DIR
            NAMES gio/gio.h
            PATH_SUFFIXES glib-2.0
            HINTS ${CMAKE_INSTALL_PREFIX} ${CMAKE_PREFIX_PATH}
            ${glib_find_args})
  find_library(GOBJECT_LIBRARY NAMES gobject-2.0 ${glib_find_args})
  find_library(GIO_LIBRARY NAMES gio-2.0 ${glib_find_args})
  find_package_handle_standard_args(Gio REQUIRED_VARS GIO_LIBRARY GIO_INCLUDE_DIR GOBJECT_LIBRARY)

endif()

if(GIO_FOUND)
  set(GIO_INCLUDE_DIRS ${GIO_INCLUDE_DIR})
  set(GIO_LIBRARIES ${GIO_LIBRARY} ${GOBJECT_LIBRARY})
endif()
