if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

include_directories(${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/win)

configure_file(resdefs.h.in ${CMAKE_CURRENT_BINARY_DIR}/resdefs.h)

add_subdirectory(rfb_win32)

if(BUILD_WINVNC)
add_subdirectory(vncconfig)
add_subdirectory(winvnc)
add_subdirectory(wm_hooks)
endif()
