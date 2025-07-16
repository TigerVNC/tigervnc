#[=======================================================================[.rst:
FindPAM
-------

Find the Pluggable Authentication Modules library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``PAM_INCLUDE_DIRS``
  where to find security/pam_appl.h, etc.
``PAM_LIBRARIES``
  the libraries to link against to use PAM.
``PAM_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PAM QUIET pam)
endif()

find_path(PAM_INCLUDE_DIR NAMES security/pam_appl.h
  HINTS
    ${PC_PAM_INCLUDE_DIRS}
)
mark_as_advanced(PAM_INCLUDE_DIR)

find_library(PAM_LIBRARY NAMES pam
  HINTS
    ${PC_PAM_LIBRARY_DIRS}
)
mark_as_advanced(PAM_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PAM
  REQUIRED_VARS
    PAM_LIBRARY PAM_INCLUDE_DIR
)

if(PAM_FOUND)
  set(PAM_INCLUDE_DIRS ${PAM_INCLUDE_DIR})
  set(PAM_LIBRARIES ${PAM_LIBRARY})
endif()
