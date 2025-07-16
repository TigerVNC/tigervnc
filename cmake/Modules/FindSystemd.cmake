#[=======================================================================[.rst:
FindSystemd
-----------

Find the systemd library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SYSTEMD_INCLUDE_DIRS``
  where to find systemd/sd-daemon.h, etc.
``SYSTEMD_LIBRARIES``
  the libraries to link against to use libsystemd.
``SYSTEMD_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Systemd QUIET libsystemd)
endif()

find_path(Systemd_INCLUDE_DIR NAMES systemd/sd-daemon.h
  HINTS
    ${PC_Systemd_INCLUDE_DIRS}
)
mark_as_advanced(Systemd_INCLUDE_DIR)

find_library(Systemd_LIBRARY NAMES systemd
  HINTS
    ${PC_Systemd_LIBRARY_DIRS}
)
mark_as_advanced(Systemd_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Systemd
  REQUIRED_VARS
    Systemd_LIBRARY Systemd_INCLUDE_DIR
)

if(Systemd_FOUND)
  set(SYSTEMD_INCLUDE_DIRS ${Systemd_INCLUDE_DIR})
  set(SYSTEMD_LIBRARIES ${Systemd_LIBRARY})
endif()
