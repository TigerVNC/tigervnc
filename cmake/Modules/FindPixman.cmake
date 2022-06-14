find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
	pkg_check_modules(PIXMAN pixman-1)
else()
	find_path(PIXMAN_INCLUDE_DIRS NAMES pixman.h PATH_SUFFIXES pixman-1)
	find_library(PIXMAN_LIBRARIES NAMES pixman-1)
	find_package_handle_standard_args(PIXMAN DEFAULT_MSG PIXMAN_LIBRARIES PIXMAN_INCLUDE_DIRS)
endif()
