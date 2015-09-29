include_directories(${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/win)

configure_file(resdefs.h.in ${CMAKE_CURRENT_BINARY_DIR}/resdefs.h)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-D_DEBUG)
ENDIF()

add_subdirectory(rfb_win32)

if(BUILD_WINVNC)
add_subdirectory(vncconfig)
add_subdirectory(winvnc)
add_subdirectory(wm_hooks)
endif()
