#[=======================================================================[.rst:
FindUuid
----------
Find the UUID library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``UUID_INCLUDE_DIRS``
  where to find uuid/uuid.h, etc.
``UUID_LIBRARIES``
  the libraries to link against to use libuuid.
``UUID_FOUND``
  TRUE if found
#]=======================================================================]

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Uuid QUIET uuid)
endif()

find_path(Uuid_INCLUDE_DIR NAMES uuid/uuid.h
  HINTS
    ${PC_Uuid_INCLUDE_DIRS}
)
mark_as_advanced(Uuid_INCLUDE_DIR)

find_library(Uuid_LIBRARIES NAMES uuid
  HINTS
    ${PC_Uuid_LIBRARY_DIRS}
)
mark_as_advanced(Uuid_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Uuid
  REQUIRED_VARS
    Uuid_LIBRARIES Uuid_INCLUDE_DIR
)

if(Uuid_FOUND)
  set(UUID_INCLUDE_DIRS ${Uuid_INCLUDE_DIR})
  set(UUID_LIBRARIES ${Uuid_LIBRARIES})
endif()
