find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(PIPEWIRE libpipewire-0.3)
endif()

if(NOT PIPEWIRE_NOT_FOUND)
  find_path(PIPEWIRE_INCLUDE_DIR
            NAMES pipewire/pipewire.h
            PATH_SUFFIXES pipewire-0.3)
  find_path(SPA_INCLUDE_DIR
            NAMES spa/pod/pod.h
            PATH_SUFFIXES spa-0.2)
  find_library(PIPEWIRE_LIBRARY NAMES pipewire-0.3)
  find_package_handle_standard_args(Pipewire
                                    REQUIRED_VARS PIPEWIRE_LIBRARY PIPEWIRE_INCLUDE_DIR SPA_INCLUDE_DIR
                                    VERSION_VAR PIPEWIRE_VERSION
                                    HANDLE_COMPONENTS)

endif()

if(Pipewire_FOUND)
  set(PIPEWIRE_LIBRARIES ${PIPEWIRE_LIBRARY})
  set(PIPEWIRE_INCLUDE_DIRS ${PIPEWIRE_INCLUDE_DIR} ${SPA_INCLUDE_DIR})
endif()
