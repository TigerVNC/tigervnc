#[=======================================================================[.rst:
FindAvahi
----------

Finds Avahi library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``AVAHI_INCLUDE_DIRS``
  where to find avahi-client/publish.h, etc.
``AVAHI_LIBRARIES``
  the libraries to link against to use Avahi.
``AVAHI_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PC_Avahi QUIET avahi-client)
endif()

find_path(Avahi_INCLUDE_DIR NAMES avahi-client/publish.h
  HINTS
    ${PC_Avahi_INCLUDE_DIRS}
)
mark_as_advanced(Avahi_INCLUDE_DIR)

find_library(AvahiCommon_LIBRARY NAMES avahi-common
  HINTS
    ${PC_Avahi_LIBRARY_DIRS}
)
mark_as_advanced(AvahiCommon_LIBRARY)

find_library(AvahiClient_LIBRARY NAMES avahi-client
  HINTS
    ${PC_Avahi_LIBRARY_DIRS}
)
mark_as_advanced(AvahiClient_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Avahi
  REQUIRED_VARS
    AvahiClient_LIBRARY AvahiCommon_LIBRARY Avahi_INCLUDE_DIR
)

if(Avahi_FOUND)
  set(AVAHI_INCLUDE_DIRS ${Avahi_INCLUDE_DIR})
  set(AVAHI_LIBRARIES ${AvahiCommon_LIBRARY} ${AvahiClient_LIBRARY})
endif()
