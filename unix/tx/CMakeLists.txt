if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_library(tx STATIC
  TXWindow.cxx)

target_include_directories(tx PUBLIC ${CMAKE_SOURCE_DIR}/unix/tx)

target_link_libraries(tx PRIVATE core)
target_include_directories(tx SYSTEM PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(tx PRIVATE ${X11_LIBRARIES})
