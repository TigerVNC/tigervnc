function(create_find_module target)
  message("-- Creating CMake find module for target ${target}")

  # No support for shared libraries, as TigerVNC only needs find modules
  # for static libraries.
  get_target_property(_target_type ${target} TYPE)
  if("${_target_type}" MATCHES "^[^STATIC_LIBRARY]$")
    message(FATAL_ERROR " -  trying to use create_find_module for non-static library target.")
  endif()

  get_target_property(_binary_dir ${target} BINARY_DIR)
  set(_modname Find${target}.cmake)
  set(_outname ${_binary_dir}/${_modname})

  file(WRITE ${_outname} "# ${_modname} - Generated find module for ${target}\n")
  file(APPEND ${_outname} "\n")

  file(APPEND ${_outname} "include(CMakeFindDependencyMacro)\n")
  get_property(target_libs TARGET ${target}
               PROPERTY INTERFACE_LINK_LIBRARIES)
  foreach(library ${target_libs})
    if(TARGET ${library})
      file(APPEND ${_outname} "find_dependency(${library})\n")
    endif()
  endforeach()
  file(APPEND ${_outname} "\n")

  file(APPEND ${_outname} "add_library(${target} STATIC IMPORTED)\n")
  file(APPEND ${_outname} "\n")
  file(APPEND ${_outname} "set_target_properties(${target} PROPERTIES\n")
  file(APPEND ${_outname} "  IMPORTED_LINK_INTERFACE_LANGUAGES \"CXX\"\n")
  set(_libname ${CMAKE_STATIC_LIBRARY_PREFIX}${target}${CMAKE_STATIC_LIBRARY_SUFFIX})
  file(APPEND ${_outname} "  IMPORTED_LOCATION \"${_binary_dir}/${_libname}\"\n")
  get_property(_inc_dirs TARGET ${target}
               PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
  file(APPEND ${_outname} "  INTERFACE_INCLUDE_DIRECTORIES \"${_inc_dirs}\"\n")
  get_property(_sys_inc_dirs TARGET ${target}
               PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
  file(APPEND ${_outname} "  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES \"${_sys_inc_dirs}\"\n")
  get_property(_link_libs TARGET ${target}
               PROPERTY INTERFACE_LINK_LIBRARIES)
  file(APPEND ${_outname} "  INTERFACE_LINK_LIBRARIES \"${_link_libs}\"\n")
  file(APPEND ${_outname} ")\n")
  file(APPEND ${_outname} "\n")
  file(APPEND ${_outname} "set(${target}_LIBRARIES ${target})\n")
  file(APPEND ${_outname} "\n")
  file(APPEND ${_outname} "include(FindPackageHandleStandardArgs)\n")
  file(APPEND ${_outname} "find_package_handle_standard_args(\n")
  file(APPEND ${_outname} "  ${target}\n")
  file(APPEND ${_outname} "  REQUIRED_VARS ${target}_LIBRARIES)\n")
endfunction()
