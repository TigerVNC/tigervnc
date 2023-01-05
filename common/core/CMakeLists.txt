add_library(core STATIC
  Configuration.cxx
  Exception.cxx
  Logger.cxx
  Logger_file.cxx
  Logger_stdio.cxx
  LogWriter.cxx
  Region.cxx
  Timer.cxx
  string.cxx
  time.cxx)

target_link_libraries(core os)
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(core SYSTEM PUBLIC ${PIXMAN_INCLUDE_DIRS})
target_link_libraries(core ${PIXMAN_LIBRARIES})
target_link_directories(core PUBLIC ${PIXMAN_LIBRARY_DIRS})

if(WIN32)
  target_link_libraries(core ws2_32)
endif()

if(UNIX)
  target_sources(core PRIVATE Logger_syslog.cxx)
endif()

if(UNIX)
  libtool_create_control_file(core)
endif()
