#[=======================================================================[.rst:
FindSWScale
-----------

Find the FFmpeg swscale library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SWSCALE_INCLUDE_DIRS``
  where to find libswscale/swscale.h, etc.
``SWSCALE_LIBRARIES``
  the libraries to link against to use swscale.
``SWSCALE_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
	pkg_check_modules(SWSCALE libswscale)
else()
	find_path(SWSCALE_INCLUDE_DIRS NAMES libswscale/swscale.h)
	find_library(SWSCALE_LIBRARIES NAMES swscale)
	find_package_handle_standard_args(SWSCALE DEFAULT_MSG SWSCALE_LIBRARIES SWSCALE_INCLUDE_DIRS)
endif()

if(SWScale_FIND_REQUIRED AND NOT SWSCALE_FOUND)
	message(FATAL_ERROR "Could not find libswscale")
endif()
