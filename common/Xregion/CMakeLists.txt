add_library(Xregion STATIC
  Region.c)

if(UNIX)
  libtool_create_control_file(Xregion)
endif()
