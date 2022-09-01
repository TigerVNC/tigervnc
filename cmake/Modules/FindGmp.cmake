find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(GMP gmp)
endif()

# Only very recent versions of gmp has pkg-config support, so we have to
# fall back on a more classical search
if(NOT GMP_FOUND)
  find_path(GMP_INCLUDE_DIRS NAMES gmp.h PATH_SUFFIXES)
  find_library(GMP_LIBRARIES NAMES gmp)
  find_package_handle_standard_args(GMP DEFAULT_MSG GMP_LIBRARIES GMP_INCLUDE_DIRS)
endif()
