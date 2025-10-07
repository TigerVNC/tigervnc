#[=======================================================================[.rst:
FindXkbcommon
----------
Find the Xkbcommon library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``XKBCOMMON_INCLUDE_DIRS``
  where to find xkbcommon/xkbcommon.h, etc.
``XKBCOMMON_LIBRARIES``
  the libraries to link against to use libxkbcommon.
``XKBCOMMON_FOUND``
  TRUE if found
#]=======================================================================]


find_package(Pkgconfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Xkbcommon QUIET xkbcommon)
endif()

find_path(Xkbcommon_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h
  HINTS
    ${PC_Xkbcommon_INCLUDE_DIRS}
)
mark_as_advanced(Xkbcommon_INCLUDE_DIR)

find_library(Xkbcommon_LIBRARIES NAMES xkbcommon
  HINTS
    ${PC_Xkbcommon_LIBRARY_DIRS}
)
mark_as_advanced(Xkbcommon_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Xkbcommon
  REQUIRED_VARS Xkbcommon_LIBRARIES Xkbcommon_INCLUDE_DIR
)

if(Xkbcommon_FOUND)
  set(XKBCOMMON_INCLUDE_DIRS ${Xkbcommon_INCLUDE_DIR})
  set(XKBCOMMON_LIBRARIES ${Xkbcommon_LIBRARIES})
endif()
