include_directories(${X11_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/common/rfb)

add_library(tx STATIC
  TXWindow.cxx)

target_link_libraries(tx ${X11_LIBRARIES})
