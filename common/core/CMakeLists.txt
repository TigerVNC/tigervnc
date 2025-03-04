file(GLOB HEAD_FILES *.h)
set(INSTALL_HEAD_FILES ${HEAD_FILES})
add_library(core OBJECT
  ${INSTALL_HEAD_FILES}
  Configuration.cxx
  Exception.cxx
  Logger.cxx
  Logger_file.cxx
  Logger_stdio.cxx
  LogWriter.cxx
  Mutex.cxx
  Region.cxx
  Timer.cxx
  Thread.cxx
  string.cxx
  time.cxx
  xdgdirs.cxx)

target_include_directories(core PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/common>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)
target_include_directories(core PRIVATE ${PIXMAN_INCLUDE_DIRS})
target_link_libraries(core ${PIXMAN_LIBRARIES})
target_link_directories(core PUBLIC ${PIXMAN_LIBRARY_DIRS})

if(UNIX)
  target_sources(core PRIVATE Logger_syslog.cxx)
  target_link_libraries(core pthread)
endif()

if(WIN32)
  target_link_libraries(core ws2_32)
endif()

if(UNIX)
  target_sources(core PRIVATE Logger_syslog.cxx)
endif()

# Set install head files
set_target_properties(core PROPERTIES
  PUBLIC_HEADER "${INSTALL_HEAD_FILES}")
# Install head files
INSTALL(TARGETS core
  EXPORT tigervncConfig
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc/core
    COMPONENT Development
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tigervnc
  )
