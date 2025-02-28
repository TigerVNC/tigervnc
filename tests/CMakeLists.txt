if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

# Benchmarking tools
add_subdirectory(perf)

# Unit tests
if(GTest_FOUND)
  add_subdirectory(unit)
endif()
