#[=======================================================================[.rst:
FindPipeWire
----------
Find the PipeWire library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``PIPEWIRE_INCLUDE_DIRS``
  where to find pipewire/pipewire.h, etc.
``PIPEWIRE_LIBRARIES``
  the libraries to link against to use libpipewire.
``PIPEWIRE_FOUND``
  TRUE if found
#]=======================================================================]


find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PipeWire QUIET libpipewire-0.3)
endif()

find_path(PipeWire_INCLUDE_DIR NAMES pipewire/pipewire.h
  PATH_SUFFIXES
    pipewire-0.3
  HINTS
    ${PC_PipeWire_INCLUDE_DIRS}
)
mark_as_advanced(PipeWire_INCLUDE_DIR)

find_path(Spa_INCLUDE_DIR NAMES spa/pod/pod.h
  PATH_SUFFIXES
    spa-0.2
   HINTS
    ${PC_PipeWire_INCLUDE_DIRS})
mark_as_advanced(Spa_INCLUDE_DIR)

find_library(PipeWire_LIBRARY NAMES pipewire-0.3
  HINTS
    ${PC_PipeWire_LIBRARY_DIRS}
)
mark_as_advanced(PipeWire_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PipeWire
  REQUIRED_VARS
    PipeWire_LIBRARY PipeWire_INCLUDE_DIR Spa_INCLUDE_DIR
)

if(PipeWire_FOUND)
  set(PIPEWIRE_LIBRARIES ${PipeWire_LIBRARY})
  set(PIPEWIRE_INCLUDE_DIRS ${PipeWire_INCLUDE_DIR} ${Spa_INCLUDE_DIR})
endif()
