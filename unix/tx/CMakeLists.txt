if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_library(tx STATIC
  TXWindow.cxx)

target_include_directories(tx SYSTEM PUBLIC ${X11_INCLUDE_DIR})

target_include_directories(tx PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(tx PUBLIC ${CMAKE_SOURCE_DIR}/common/rfb)

target_link_libraries(tx core ${X11_LIBRARIES})
