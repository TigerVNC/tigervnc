#[=======================================================================[.rst:
FindGio
----------
Find the GIO library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``GIO_INCLUDE_DIRS``
  where to find gio/gio.h, etc.
``GIO_LIBRARIES``
  the libraries to link against to use libgio.
``GIO_FOUND``
  TRUE if found
#]=======================================================================]

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Gio QUIET gio-2.0)
  pkg_check_modules(PC_Gio_unix QUIET gio-unix-2.0)
endif()

find_path(Gio_INCLUDE_DIR NAMES gio/gio.h
  PATH_SUFFIXES
    glib-2.0
  HINTS
    ${PC_Gio_INCLUDE_DIRS}
)
mark_as_advanced(Gio_INCLUDE_DIR)

find_path(Gio_unix_INCLUDE_DIR NAMES gio/gunixfdlist.h
  PATH_SUFFIXES
    gio-unix-2.0
  HINTS
    ${PC_Gio_unix_INCLUDE_DIRS}
)
mark_as_advanced(Gio_unix_INCLUDE_DIR)

find_library(Gio_LIBRARIES NAMES gio-2.0
  HINTS
    ${PC_Gio_LIBRARY_DIRS}
)
mark_as_advanced(Gio_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gio
  REQUIRED_VARS
    Gio_LIBRARIES Gio_INCLUDE_DIR Gio_unix_INCLUDE_DIR
)


if(GIO_FOUND)
  set(GIO_INCLUDE_DIRS ${Gio_INCLUDE_DIR} ${Gio_unix_INCLUDE_DIR})
  set(GIO_LIBRARIES ${Gio_LIBRARIES})
endif()
