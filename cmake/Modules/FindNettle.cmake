#[=======================================================================[.rst:
FindNettle
----------

Find the Nettle and Hogweed libraries

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``NETTLE_INCLUDE_DIRS``
  where to find eax.h, etc.
``NETTLE_LIBRARIES``
  the libraries to link against to use Nettle.
``HOGWEED_LIBRARIES``
  the libraries to link against to use Hogweed.
``NETTLE_FOUND``
  TRUE if found

#]=======================================================================]

find_package(GMP)
find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(NETTLE nettle>=3.0)
  pkg_check_modules(HOGWEED hogweed)
else()
  find_path(NETTLE_INCLUDE_DIRS NAMES eax.h PATH_SUFFIXES nettle)
  find_library(NETTLE_LIBRARIES NAMES nettle)
  find_package_handle_standard_args(NETTLE DEFAULT_MSG NETTLE_LIBRARIES NETTLE_INCLUDE_DIRS)
  find_library(HOGWEED_LIBRARIES NAMES hogweed)
  find_package_handle_standard_args(HOGWEED DEFAULT_MSG HOGWEED_LIBRARIES)
endif()

if (NOT HOGWEED_FOUND OR NOT GMP_FOUND)
  set(NETTLE_FOUND 0)
endif()

if(Nettle_FIND_REQUIRED AND NOT NETTLE_FOUND)
	message(FATAL_ERROR "Could not find Nettle")
endif()
