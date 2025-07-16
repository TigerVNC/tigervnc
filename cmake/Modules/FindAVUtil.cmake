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

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_AVUtil QUIET libavutil)
endif()

find_path(AVUtil_INCLUDE_DIR NAMES libavutil/avutil.h
  HINTS
    ${PC_AVUtil_INCLUDE_DIRS}
)
mark_as_advanced(AVUtil_INCLUDE_DIR)

find_library(AVUtil_LIBRARY NAMES avutil
  HINTS
    ${PC_AVUtil_LIBRARY_DIRS}
)
mark_as_advanced(AVUtil_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVUtil
  REQUIRED_VARS
    AVUtil_LIBRARY AVUtil_INCLUDE_DIR
)

if(AVUtil_FOUND)
  set(AVUTIL_INCLUDE_DIRS ${AVUtil_INCLUDE_DIR})
  set(AVUTIL_LIBRARIES ${AVUtil_LIBRARY})
endif()
