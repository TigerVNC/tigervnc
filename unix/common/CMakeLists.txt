add_library(unixcommon STATIC
  randr.cxx)

target_include_directories(unixcommon PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(unixcommon PUBLIC ${CMAKE_SOURCE_DIR}/unix/common)

if(UNIX)
  libtool_create_control_file(unixcommon)
endif()

if(NOT WIN32)
  set_target_properties(unixcommon
    PROPERTIES COMPILE_FLAGS -fPIC)
endif()
