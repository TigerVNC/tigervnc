#[=======================================================================[.rst:
FindNettle
----------

Find the Nettle and Hogweed libraries

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``NETTLE_INCLUDE_DIRS``
  where to find nettle/eax.h, etc.
``NETTLE_LIBRARIES``
  the libraries to link against to use Nettle.
``HOGWEED_LIBRARIES``
  the libraries to link against to use Hogweed.
``NETTLE_FOUND``
  TRUE if found

#]=======================================================================]

if(Nettle_FIND_REQUIRED)
  find_package(GMP QUIET REQUIRED)
else()
  find_package(GMP QUIET)
endif()

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Nettle QUIET nettle)
  pkg_check_modules(PC_Hogweed QUIET hogweed)
endif()

find_path(Nettle_INCLUDE_DIR NAMES nettle/eax.h
  HINTS
    ${PC_Nettle_INCLUDE_DIRS}
)
mark_as_advanced(Nettle_INCLUDE_DIR)

find_library(Nettle_LIBRARY NAMES nettle
  HINTS
    ${PC_Nettle_LIBRARY_DIRS}
)
mark_as_advanced(Nettle_LIBRARY)

find_library(Hogweed_LIBRARY NAMES hogweed
  HINTS
    ${PC_Hogweed_LIBRARY_DIRS}
)
mark_as_advanced(Hogweed_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Nettle
  REQUIRED_VARS
    Nettle_LIBRARY Nettle_INCLUDE_DIR Hogweed_LIBRARY
)

if(Nettle_FOUND)
  set(NETTLE_INCLUDE_DIRS ${Nettle_INCLUDE_DIR} ${GMP_INCLUDE_DIRS})
  set(NETTLE_LIBRARIES ${Nettle_LIBRARY})
  set(HOGWEED_LIBRARIES ${Hogweed_LIBRARY} ${GMP_LIBRARIES})
endif()
