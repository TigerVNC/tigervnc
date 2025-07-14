cmake_parse_arguments(
  GLIB_FIND_ARGS
  "REQUIRED;OPTIONAL;QUIET"
  ""
  ""
  ${ARGN}
)

set(glib_find_args "")

if(GLIB_FIND_ARGS_QUIET)
  list(APPEND glib_find_args QUIET)
endif()

if (GLIB_FIND_ARGS_REQUIRED)
  list(APPEND glib_find_args REQUIRED)
elseif(GLIB_FIND_ARGS_OPTIONAL)
  list(APPEND glib_find_args OPTIONAL)
endif()

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(GLIB glib-2.0 ${glib_find_args})
endif()

if(NOT GLIB_FOUND)
  find_path(GLIB_INCLUDE_DIR
            NAMES glib.h
            PATH_SUFFIXES glib-2.0
            HINTS ${CMAKE_INSTALL_PREFIX} ${CMAKE_PREFIX_PATH}
            ${glib_find_args})
  find_path(GLIB_CONFIG_INCLUDE_DIR
            NAMES glibconfig.h
            PATH_SUFFIXES glib-2.0/include
            HINTS ${CMAKE_INSTALL_PREFIX} ${CMAKE_PREFIX_PATH}
            ${glib_find_args})
  find_library(GLIB_LIBRARIES NAMES glib-2.0 ${glib_find_args})
  find_package_handle_standard_args(GLib REQUIRED_VARS GLIB_LIBRARIES GLIB_INCLUDE_DIR GLIB_CONFIG_INCLUDE_DIR)
endif()

if(GLIB_FOUND)
  set(GLIB_INCLUDE_DIRS ${GLIB_INCLUDE_DIR} ${GLIB_CONFIG_INCLUDE_DIR})
endif()