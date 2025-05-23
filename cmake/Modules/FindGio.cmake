find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(GIO gio-2.0)
endif()

find_package(GLib)

if(NOT GIO_FOUND)
  find_path(GIO_INCLUDE_DIR
            NAMES gio/gio.h
            PATH_SUFFIXES glib-2.0)

  find_library(GOBJECT_LIBRARY NAMES gobject-2.0)
  find_library(GIO_LIBRARY NAMES gio-2.0)

  find_package_handle_standard_args(Gio
                                    REQUIRED_VARS GIO_LIBRARY GIO_INCLUDE_DIR GOBJECT_LIBRARY
                                    VERSION_VAR GIO_VERSION
                                    HANDLE_COMPONENTS)

endif()

if(GIO_FOUND)
  set(GIO_INCLUDE_DIRS ${GIO_INCLUDE_DIR})
  set(GIO_LIBRARIES ${GIO_LIBRARY} ${GOBJECT_LIBRARY})
endif()
