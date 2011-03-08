include_directories(${CMAKE_SOURCE_DIR}/common)

add_executable(vncpasswd 
  vncpasswd.cxx)

target_link_libraries(vncpasswd tx rfb rdr os)
