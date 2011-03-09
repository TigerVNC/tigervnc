add_subdirectory(tx)

add_subdirectory(vncconfig)
add_subdirectory(vncpasswd)

if(NOT BUILD_NEW_VNCVIEWER)
add_subdirectory(vncviewer)
endif()
