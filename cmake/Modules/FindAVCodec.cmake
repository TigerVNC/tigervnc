#[=======================================================================[.rst:
FindAVCodec
-----------

Find the FFmpeg avcodec library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``AVCODEC_INCLUDE_DIRS``
  where to find avcodec.h, etc.
``AVCODEC_LIBRARIES``
  the libraries to link against to use avcodec.
``AVCODEC_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
	pkg_check_modules(AVCODEC libavcodec)
else()
	find_path(AVCODEC_INCLUDE_DIRS NAMES avcodec.h PATH_SUFFIXES libavcodec)
	find_library(AVCODEC_LIBRARIES NAMES avcodec)
	find_package_handle_standard_args(AVCODEC DEFAULT_MSG AVCODEC_LIBRARIES AVCODEC_INCLUDE_DIRS)
endif()

if(AVCodec_FIND_REQUIRED AND NOT AVCODEC_FOUND)
	message(FATAL_ERROR "Could not find libavcodec")
endif()
