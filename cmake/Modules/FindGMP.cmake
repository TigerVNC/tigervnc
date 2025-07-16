#[=======================================================================[.rst:
FindGMP
-------

Find the GNU MP bignum library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``GMP_INCLUDE_DIRS``
  where to find gmp.h, etc.
``GMP_LIBRARIES``
  the libraries to link against to use GMP.
``GMP_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GMP QUIET gmp)
endif()

find_path(GMP_INCLUDE_DIR NAMES gmp.h
  HINTS
    ${PC_GMP_INCLUDE_DIRS}
)
mark_as_advanced(GMP_INCLUDE_DIR)

find_library(GMP_LIBRARY NAMES gmp
  HINTS
    ${PC_GMP_LIBRARY_DIRS}
)
mark_as_advanced(GMP_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP
  REQUIRED_VARS
    GMP_LIBRARY GMP_INCLUDE_DIR
)

if(GMP_FOUND)
  set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})
  set(GMP_LIBRARIES ${GMP_LIBRARY})
endif()
