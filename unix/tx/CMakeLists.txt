add_library(tx STATIC
  TXWindow.cxx)

target_include_directories(tx SYSTEM PUBLIC ${X11_INCLUDE_DIR})

target_include_directories(tx PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(tx PUBLIC ${CMAKE_SOURCE_DIR}/common/rfb)

target_link_libraries(tx ${X11_LIBRARIES})
