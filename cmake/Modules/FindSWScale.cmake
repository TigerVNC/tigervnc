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

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SWScale QUIET libswscale)
endif()

find_path(SWScale_INCLUDE_DIR NAMES libswscale/swscale.h
  HINTS
    ${PC_SWScale_INCLUDE_DIRS}
)
mark_as_advanced(SWScale_INCLUDE_DIR)

find_library(SWScale_LIBRARY NAMES swscale
  HINTS
    ${PC_SWScale_LIBRARY_DIRS}
)
mark_as_advanced(SWScale_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWScale
  REQUIRED_VARS
    SWScale_LIBRARY SWScale_INCLUDE_DIR
)

if(SWScale_FOUND)
  set(SWSCALE_INCLUDE_DIRS ${SWScale_INCLUDE_DIR})
  set(SWSCALE_LIBRARIES ${SWScale_LIBRARY})
endif()
