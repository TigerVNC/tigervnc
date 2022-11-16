add_library(core STATIC
  Region.cxx
  util.cxx)

target_link_libraries(core rfb)
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(core SYSTEM PUBLIC ${PIXMAN_INCLUDE_DIRS})
target_link_libraries(core ${PIXMAN_LIBRARIES})
target_link_directories(core PUBLIC ${PIXMAN_LIBRARY_DIRS})

if(UNIX)
  libtool_create_control_file(core)
endif()
