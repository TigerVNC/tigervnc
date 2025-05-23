#[=======================================================================[.rst:
FindGLib
----------
Find the GLib library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``GLIB_INCLUDE_DIRS``
  where to find glib.h, etc.
``GLIB_LIBRARIES``
  the libraries to link against to use libglib.
``GLIB_FOUND``
  TRUE if found
#]=======================================================================]


find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GLib QUIET glib-2.0)
endif()

find_path(GLib_INCLUDE_DIR NAMES glib.h
  PATH_SUFFIXES
    glib-2.0
  HINTS
    ${PC_GLib_INCLUDE_DIRS}
)
mark_as_advanced(GLib_INCLUDE_DIR)

find_path(GLib_CONFIG_INCLUDE_DIR NAMES glibconfig.h
  PATH_SUFFIXES
    glib-2.0/include
  HINTS
    ${PC_GLib_INCLUDE_DIRS}
)
mark_as_advanced(GLib_CONFIG_INCLUDE_DIR)

find_library(GLib_LIBRARIES NAMES glib-2.0
  HINTS
    ${PC_GLib_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLib
  REQUIRED_VARS
    GLib_LIBRARIES GLib_INCLUDE_DIR GLib_CONFIG_INCLUDE_DIR
)

if(GLIB_FOUND)
  set(GLIB_INCLUDE_DIRS ${GLib_INCLUDE_DIR} ${GLib_CONFIG_INCLUDE_DIR})
  set(GLIB_LIBRARIES ${GLib_LIBRARIES})
endif()
