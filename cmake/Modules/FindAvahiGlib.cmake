#[=======================================================================[.rst:
FindAvahiGlib
----------

Finds Avahi Glib integration library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``AVAHI_GLIB_INCLUDE_DIRS``
  where to find avahi-glib/glib-watch.h, etc.
``AVAHI_GLIB_LIBRARIES``
  the libraries to link against to use Glib integration.
``AVAHI_GLIB_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PC_AvahiGlib QUIET avahi-glib)
endif()

find_path(AvahiGlib_INCLUDE_DIR NAMES avahi-glib/glib-watch.h
  HINTS
    ${PC_AvahiGlib_INCLUDE_DIRS}
)
mark_as_advanced(AvahiGlib_INCLUDE_DIR)

find_library(AvahiGlib_LIBRARY NAMES avahi-glib
  HINTS
    ${PC_AvahiGlib_LIBRARY_DIRS}
)
mark_as_advanced(AvahiGlib_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AvahiGlib
  REQUIRED_VARS
    AvahiGlib_LIBRARY AvahiGlib_INCLUDE_DIR
)

if(AvahiGlib_FOUND)
  set(AVAHI_GLIB_FOUND TRUE)
  set(AVAHI_GLIB_INCLUDE_DIRS ${AvahiGlib_INCLUDE_DIR})
  set(AVAHI_GLIB_LIBRARIES ${AvahiGlib_LIBRARY})
endif()
