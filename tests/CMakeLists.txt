include_directories(${CMAKE_SOURCE_DIR}/common)

add_library(test_util STATIC util.cxx)

add_executable(pixelconv pixelconv.cxx)
target_link_libraries(pixelconv test_util rfb)
