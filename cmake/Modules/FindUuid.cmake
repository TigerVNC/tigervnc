cmake_parse_arguments(
  UUID_FIND_ARGS
  "REQUIRED;OPTIONAL;QUIET"
  ""
  ""
  ${ARGN}
)

set(uuid_find_args "")

if(UUID_FIND_ARGS_QUIET)
  list(APPEND uuid_find_args QUIET)
endif()

if (UUID_FIND_ARGS_REQUIRED)
  list(APPEND uuid_find_args REQUIRED)
elseif(UUID_FIND_ARGS_OPTIONAL)
  list(APPEND uuid_find_args OPTIONAL)
endif()

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(UUID uuid)
endif()

if (NOT UUID_FOUND)
  find_path(UUID_INCLUDE_DIRS
            NAMES uuid/uuid.h
            HINTS ${CMAKE_INSTALL_PREFIX} ${CMAKE_PREFIX_PATH}
            ${uuid_find_args})
  find_library(UUID_LIBRARIES NAMES uuid ${uuid_find_args})
  find_package_handle_standard_args(Uuid REQUIRED_VARS UUID_LIBRARIES UUID_INCLUDE_DIRS)
endif()
