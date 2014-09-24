include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(test_util STATIC util.cxx)

add_executable(convperf convperf.cxx)
target_link_libraries(convperf test_util rfb)
