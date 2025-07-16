#[=======================================================================[.rst:
FindAVUtil
----------

Find the FFmpeg avutil library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``AVUTIL_INCLUDE_DIRS``
  where to find libavutil/avutil.h, etc.
``AVUTIL_LIBRARIES``
  the libraries to link against to use avutil.
``AVUTIL_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
	pkg_check_modules(AVUTIL libavutil)
else()
	find_path(AVUTIL_INCLUDE_DIRS NAMES libavutil/avutil.h)
	find_library(AVUTIL_LIBRARIES NAMES avutil)
	find_package_handle_standard_args(AVUTIL DEFAULT_MSG AVUTIL_LIBRARIES AVUTIL_INCLUDE_DIRS)
endif()

if(AVUtil_FIND_REQUIRED AND NOT AVUTIL_FOUND)
	message(FATAL_ERROR "Could not find libavutil")
endif()
