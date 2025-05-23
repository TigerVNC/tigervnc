find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(GLIB glib-2.0)
endif()

if(NOT GLIB_FOUND)
  find_path(GLIB_INCLUDE_DIR
            NAMES glib.h
            PATH_SUFFIXES glib-2.0)

  find_path(GLIB_CONFIG_INCLUDE_DIR
            NAMES glibconfig.h
            PATH_SUFFIXES glib-2.0/include
            HINTS /usr/lib /usr/lib64
  )

  find_library(GLIB_LIBRARIES NAMES glib-2.0)

  find_package_handle_standard_args(GLib
                                    REQUIRED_VARS GLIB_LIBRARIES GLIB_INCLUDE_DIR GLIB_CONFIG_INCLUDE_DIR
                                    VERSION_VAR GLIB_VERSION
                                    HANDLE_COMPONENTS)
endif()

if(GLIB_FOUND)
  set(GLIB_INCLUDE_DIRS ${GLIB_INCLUDE_DIR} ${GLIB_CONFIG_INCLUDE_DIR})
endif()