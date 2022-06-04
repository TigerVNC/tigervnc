# Compatibility replacement of target_link_directories() for older cmake

if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
  function(target_link_directories TARGET SCOPE)
    get_target_property(INTERFACE_LINK_LIBRARIES ${TARGET} INTERFACE_LINK_LIBRARIES)
    foreach(DIRECTORY ${ARGN})
      list(INSERT INTERFACE_LINK_LIBRARIES 0 "-L${DIRECTORY}")
    endforeach()
    set_target_properties(${TARGET} PROPERTIES
      INTERFACE_LINK_LIBRARIES "${INTERFACE_LINK_LIBRARIES}")
  endfunction()
endif()
