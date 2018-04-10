include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/unix/common)

add_library(unixcommon STATIC
  randr.cxx)

if(UNIX)
  libtool_create_control_file(unixcommon)
endif()

if(NOT WIN32)
  set_target_properties(unixcommon
    PROPERTIES COMPILE_FLAGS -fPIC)
endif()
