#[=======================================================================[.rst:
FindSELinux
-----------

Find the SELinux library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SELINUX_INCLUDE_DIRS``
  where to find selinux/selinux.h, etc.
``SELINUX_LIBRARIES``
  the libraries to link against to use libselinux.
``SELINUX_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SELinux QUIET libselinux)
endif()

find_path(SELinux_INCLUDE_DIR NAMES selinux/selinux.h
  HINTS
    ${PC_SELinux_INCLUDE_DIRS}
)
mark_as_advanced(SELinux_INCLUDE_DIR)

find_library(SELinux_LIBRARY NAMES selinux
  HINTS
    ${PC_SELinux_LIBRARY_DIRS}
)
mark_as_advanced(SELinux_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SELinux
  REQUIRED_VARS
    SELinux_LIBRARY SELinux_INCLUDE_DIR
)

if(SELinux_FOUND)
  set(SELINUX_INCLUDE_DIRS ${SELinux_INCLUDE_DIR})
  set(SELINUX_LIBRARIES ${SELinux_LIBRARY})
endif()
