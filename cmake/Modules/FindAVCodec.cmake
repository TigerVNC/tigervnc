#[=======================================================================[.rst:
FindAVCodec
-----------

Find the FFmpeg avcodec library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``AVCODEC_INCLUDE_DIRS``
  where to find libavcodec/avcodec.h, etc.
``AVCODEC_LIBRARIES``
  the libraries to link against to use avcodec.
``AVCODEC_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_AVCodec QUIET libavcodec)
endif()

find_path(AVCodec_INCLUDE_DIR NAMES libavcodec/avcodec.h
  HINTS
    ${PC_AVCodec_INCLUDE_DIRS}
)
mark_as_advanced(AVCodec_INCLUDE_DIR)

find_library(AVCodec_LIBRARY NAMES avcodec
  HINTS
    ${PC_AVCodec_LIBRARY_DIRS}
)
mark_as_advanced(AVCodec_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVCodec
  REQUIRED_VARS
    AVCodec_LIBRARY AVCodec_INCLUDE_DIR
)

if(AVCodec_FOUND)
  set(AVCODEC_INCLUDE_DIRS ${AVCodec_INCLUDE_DIR})
  set(AVCODEC_LIBRARIES ${AVCodec_LIBRARY})
endif()
