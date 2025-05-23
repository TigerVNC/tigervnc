cmake_parse_arguments(
  PIPEWIRE_FIND_ARGS
  "REQUIRED;OPTIONAL;QUIET"
  ""
  ""
  ${ARGN}
)

set(pipewire_find_args "")

if(PIPEWIRE_FIND_ARGS_QUIET)
  list(APPEND pipewire_find_args QUIET)
endif()

if (PIPEWIRE_FIND_ARGS_REQUIRED)
  list(APPEND pipewire_find_args REQUIRED)
elseif(PIPEWIRE_FIND_ARGS_OPTIONAL)
  list(APPEND pipewire_find_args OPTIONAL)
endif()

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(PIPEWIRE libpipewire-0.3 ${pipewire_find_args})
endif()

if(NOT PIPEWIRE_FOUND)
  find_path(PIPEWIRE_INCLUDE_DIR
            NAMES pipewire/pipewire.h
            PATH_SUFFIXES pipewire-0.3
            HINTS ${CMAKE_INSTALL_PREFIX} ${CMAKE_PREFIX_PATH}
            ${pipewire_find_args})
  find_path(SPA_INCLUDE_DIR
            NAMES spa/pod/pod.h
            PATH_SUFFIXES spa-0.2
            ${pipewire_find_args})
  find_library(PIPEWIRE_LIBRARY NAMES pipewire-0.3 ${pipewire_find_args})
  find_package_handle_standard_args(Pipewire REQUIRED_VARS PIPEWIRE_LIBRARY PIPEWIRE_INCLUDE_DIR SPA_INCLUDE_DIR)

endif()

if(PIPEWIRE_FOUND)
  set(PIPEWIRE_LIBRARIES ${PIPEWIRE_LIBRARY})
  set(PIPEWIRE_INCLUDE_DIRS ${PIPEWIRE_INCLUDE_DIR} ${SPA_INCLUDE_DIR})
endif()
