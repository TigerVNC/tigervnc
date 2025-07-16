#[=======================================================================[.rst:
FindPWQuality
-------------

Find the password quality library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``PWQUALITY_INCLUDE_DIRS``
  where to find pwquality.h, etc.
``PWQUALITY_LIBRARIES``
  the libraries to link against to use pwquality.
``PWQUALITY_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PWQuality QUIET pwquality)
endif()

find_path(PWQuality_INCLUDE_DIR NAMES pwquality.h
  HINTS
    ${PC_PWQuality_INCLUDE_DIRS}
)
mark_as_advanced(PWQuality_INCLUDE_DIR)

find_library(PWQuality_LIBRARY NAMES pwquality
  HINTS
    ${PC_PWQuality_LIBRARY_DIRS}
)
mark_as_advanced(PWQuality_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PWQuality
  REQUIRED_VARS
    PWQuality_LIBRARY PWQuality_INCLUDE_DIR
)

if(PWQuality_FOUND)
  set(PWQUALITY_INCLUDE_DIRS ${PWQuality_INCLUDE_DIR})
  set(PWQUALITY_LIBRARIES ${PWQuality_LIBRARY})
endif()
