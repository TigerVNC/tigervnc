#[=======================================================================[.rst:
FindPixman
----------

Find the Pixman library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``PIXMAN_INCLUDE_DIRS``
  where to find pixman.h, etc.
``PIXMAN_LIBRARIES``
  the libraries to link against to use Pixman.
``PIXMAN_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Pixman QUIET pixman-1)
endif()

find_path(Pixman_INCLUDE_DIR NAMES pixman.h
  PATH_SUFFIXES
    pixman-1
  HINTS
    ${PC_Pixman_INCLUDE_DIRS}
)
mark_as_advanced(Pixman_INCLUDE_DIR)

find_library(Pixman_LIBRARY NAMES pixman-1
  HINTS
    ${PC_Pixman_LIBRARY_DIRS}
)
mark_as_advanced(Pixman_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pixman
  REQUIRED_VARS
    Pixman_LIBRARY Pixman_INCLUDE_DIR
)

if(Pixman_FOUND)
  set(PIXMAN_INCLUDE_DIRS ${Pixman_INCLUDE_DIR})
  set(PIXMAN_LIBRARIES ${Pixman_LIBRARY})
endif()
