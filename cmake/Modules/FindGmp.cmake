find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(GMP gmp)
else()
  find_path(GMP_INCLUDE_DIRS NAMES gmp.h PATH_SUFFIXES)
  find_library(GMP_LIBRARIES NAMES gmp)
  find_package_handle_standard_args(GMP DEFAULT_MSG GMP_LIBRARIES GMP_INCLUDE_DIRS)
endif()
