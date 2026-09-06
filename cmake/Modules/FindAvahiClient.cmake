#[=======================================================================[.rst:
FindAvahiClient
----------

Finds Avahi client library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``AVAHI_CLIENT_INCLUDE_DIRS``
  where to find avahi-client/publish.h, etc.
``AVAHI_CLIENT_LIBRARIES``
  the libraries to link against to use Avahi.
``AVAHI_CLIENT_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PC_AvahiClient QUIET avahi-client)
endif()

find_path(AvahiClient_INCLUDE_DIR NAMES avahi-client/publish.h
  HINTS
    ${PC_AvahiClient_INCLUDE_DIRS}
)
mark_as_advanced(AvahiClient_INCLUDE_DIR)

find_library(AvahiCommon_LIBRARY NAMES avahi-common
  HINTS
    ${PC_AvahiClient_LIBRARY_DIRS}
)
mark_as_advanced(AvahiCommon_LIBRARY)

find_library(AvahiClient_LIBRARY NAMES avahi-client
  HINTS
    ${PC_AvahiClient_LIBRARY_DIRS}
)
mark_as_advanced(AvahiClient_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AvahiClient
  REQUIRED_VARS
    AvahiClient_LIBRARY AvahiCommon_LIBRARY AvahiClient_INCLUDE_DIR
)

if(AvahiClient_FOUND)
  set(AVAHI_CLIENT_FOUND TRUE)
  set(AVAHI_CLIENT_INCLUDE_DIRS ${AvahiClient_INCLUDE_DIR})
  set(AVAHI_CLIENT_LIBRARIES ${AvahiCommon_LIBRARY} ${AvahiClient_LIBRARY})
endif()
