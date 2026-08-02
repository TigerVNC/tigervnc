#[=======================================================================[.rst:
FindPulse
---------

Find the PulseAudio client library

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``PULSE_INCLUDE_DIRS``
  where to find pulse/pulseaudio.h, etc.
``PULSE_LIBRARIES``
  the libraries to link against to use libpulse.
``PULSE_FOUND``
  TRUE if found

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Pulse QUIET libpulse)
endif()

find_path(Pulse_INCLUDE_DIR NAMES pulse/pulseaudio.h
  HINTS
    ${PC_Pulse_INCLUDE_DIRS}
)
mark_as_advanced(Pulse_INCLUDE_DIR)

find_library(Pulse_LIBRARY NAMES pulse
  HINTS
    ${PC_Pulse_LIBRARY_DIRS}
)
mark_as_advanced(Pulse_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pulse
  REQUIRED_VARS
    Pulse_LIBRARY Pulse_INCLUDE_DIR
)

if(Pulse_FOUND)
  set(PULSE_INCLUDE_DIRS ${Pulse_INCLUDE_DIR})
  set(PULSE_LIBRARIES ${Pulse_LIBRARY})
endif()
