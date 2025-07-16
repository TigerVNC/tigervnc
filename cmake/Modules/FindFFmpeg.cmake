#[=======================================================================[.rst:
FindFFmpeg
----------

Find the FFmpeg libraries

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``AVCODEC_INCLUDE_DIRS``
  where to find avcodec.h, etc.
``AVUTIL_INCLUDE_DIRS``
  where to find avutil.h, etc.
``SWSCALE_INCLUDE_DIRS``
  where to find swscale.h, etc.
``AVCODEC_LIBRARIES``
  the libraries to link against to use avcodec.
``AVUTIL_LIBRARIES``
  the libraries to link against to use avutil.
``SWSCALE_LIBRARIES``
  the libraries to link against to use swscale.
``AVCODEC_FOUND``
  TRUE if found
``AVUTIL_FOUND``
  TRUE if found
``SWSCALE_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
	pkg_check_modules(AVCODEC libavcodec)
	pkg_check_modules(AVUTIL libavutil)
	pkg_check_modules(SWSCALE libswscale)
else()
	find_path(AVCODEC_INCLUDE_DIRS NAMES avcodec.h PATH_SUFFIXES libavcodec)
	find_library(AVCODEC_LIBRARIES NAMES avcodec)
	find_package_handle_standard_args(AVCODEC DEFAULT_MSG AVCODEC_LIBRARIES AVCODEC_INCLUDE_DIRS)
	find_path(AVUTIL_INCLUDE_DIRS NAMES avutil.h PATH_SUFFIXES libavutil)
	find_library(AVUTIL_LIBRARIES NAMES avutil)
	find_package_handle_standard_args(AVUTIL DEFAULT_MSG AVUTIL_LIBRARIES AVUTIL_INCLUDE_DIRS)
	find_path(SWSCALE_INCLUDE_DIRS NAMES swscale.h PATH_SUFFIXES libswscale)
	find_library(SWSCALE_LIBRARIES NAMES swscale)
	find_package_handle_standard_args(SWSCALE DEFAULT_MSG SWSCALE_LIBRARIES SWSCALE_INCLUDE_DIRS)
endif()

if(FFmpeg_FIND_REQUIRED AND
   (NOT AVCODEC_FOUND OR NOT AVUTIL_FOUND OR NOT SWSCALE_FOUND))
	message(FATAL_ERROR "Could not find FFMPEG")
endif()
