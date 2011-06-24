include_directories(${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/win)

configure_file(resdefs.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/resdefs.h)

add_subdirectory(rfb_win32)

if(NOT BUILD_NEW_VNCVIEWER)
add_subdirectory(vncviewer)
endif()

if(BUILD_WINVNC)
add_subdirectory(vncconfig)
add_subdirectory(winvnc)
endif()
