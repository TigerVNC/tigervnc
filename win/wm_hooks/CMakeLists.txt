add_library(wm_hooks SHARED
  ../wm_hooks/wm_hooks.cxx
  ../wm_hooks/wm_hooks.rc)

# We want the DLL to be named wm_hooks.dll rather than libwm_hooks.dll
set_target_properties(wm_hooks PROPERTIES PREFIX "")

target_include_directories(wm_hooks PUBLIC ${CMAKE_BINARY_DIR}/win)
target_include_directories(wm_hooks PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS wm_hooks
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
