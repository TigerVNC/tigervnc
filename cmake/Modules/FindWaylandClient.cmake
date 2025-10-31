#[=======================================================================[.rst:
FindWaylandClient
-----------------
Find the Wayland client library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``WAYLANDCLIENT_INCLUDE_DIRS``
  where to find wayland-client.h.h, etc.
``WAYLANDCLIENT_LIBRARIES``
  the libraries to link against to use wayland client.
``WAYLAND_CLIENT_FOUND``
  TRUE if found
#]=======================================================================]

find_package(PkgConfig QUIET)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_WaylandClient QUIET wayland-client)
endif()

find_program(Wayland_scanner_EXECUTABLE
    NAMES wayland-scanner
)

find_path(WaylandClient_INCLUDE_DIR NAMES wayland-client.h
  PATH_SUFFIXES
    wayland
  HINTS
    ${PC_WaylandClient_INCLUDE_DIRS}
)
mark_as_advanced(WaylandClient_INCLUDE_DIR)

find_library(WaylandClient_LIBRARIES NAMES wayland-client
  HINTS
    ${PC_WaylandClient_LIBRARY_DIRS}
)
mark_as_advanced(WaylandClient_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WaylandClient
  REQUIRED_VARS
    WaylandClient_LIBRARIES WaylandClient_INCLUDE_DIR Wayland_scanner_EXECUTABLE
)

if(WaylandClient_FOUND)
  set(WAYLANDCLIENT_INCLUDE_DIRS ${WaylandClient_INCLUDE_DIR})
  set(WAYLANDCLIENT_LIBRARIES ${WaylandClient_LIBRARIES})
  set(WAYLAND_SCANNER_EXECUTABLE ${Wayland_scanner_EXECUTABLE})
endif()
