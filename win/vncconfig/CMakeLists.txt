include_directories(${CMAKE_BINARY_DIR}/win)

# Disable auto-generated manifests, since we have our own
if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif()

add_executable(vncconfig WIN32
  Legacy.cxx
  PasswordDialog.cxx
  vncconfig.cxx
  vncconfig.rc)

target_link_libraries(vncconfig rfb_win32 rfb Xregion network rdr ws2_32.lib)

install(TARGETS vncconfig
  RUNTIME DESTINATION .
)
