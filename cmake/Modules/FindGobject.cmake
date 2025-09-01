#[=======================================================================[.rst:
FindGobject
----------
Find the GObject library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``GOBJECT_INCLUDE_DIRS``
  where to find gobject/gobject.h, etc.
``GOBJECT_LIBRARIES``
  the libraries to link against to use libgobject.
``GOBJECT_FOUND``
  TRUE if found
#]=======================================================================]

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Gobject QUIET gobject-2.0)
endif()

find_path(Gobject_INCLUDE_DIR NAMES gobject/gobject.h
  PATH_SUFFIXES
    glib-2.0
  HINTS
    ${PC_Gobject_INCLUDE_DIRS}
)
mark_as_advanced(Gobject_INCLUDE_DIR)

find_library(Gobject_LIBRARIES NAMES gobject-2.0
  HINTS
    ${PC_Gobject_LIBRARY_DIRS}
)
mark_as_advanced(Gobject_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gobject
  REQUIRED_vARS
    Gobject_INCLUDE_DIR Gobject_LIBRARIES
)

if(GOBJECT_FOUND)
  set(GOBJECT_INCLUDE_DIRS ${Gobject_INCLUDE_DIR})
  set(GOBJECT_LIBRARIES ${Gobject_LIBRARIES})
endif()
