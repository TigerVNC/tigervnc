include_directories(${CMAKE_SOURCE_DIR}/common)

add_executable(conv conv.cxx)
target_link_libraries(conv rfb)

add_executable(convertlf convertlf.cxx)
target_link_libraries(convertlf rfb)

add_executable(hostport hostport.cxx)
target_link_libraries(hostport rfb)

add_executable(pixelformat pixelformat.cxx)
target_link_libraries(pixelformat rfb)
