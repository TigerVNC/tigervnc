# It seems like gcc for Win32 doesn't support the visibility attribute,
# so disable it to make it stop complaining.
if(WIN32)
  add_definitions(-DNO_VIZ)
endif()

add_library(zlib STATIC 
  adler32.c
  compress.c
  crc32.c
  deflate.c
  gzclose.c
  gzlib.c
  gzread.c
  gzwrite.c
  inflate.c
  infback.c
  inftrees.c
  inffast.c
  trees.c
  uncompr.c
  zutil.c)

# This code goes into libvnc.so, so it needs to be PIC on x86_64.
# (It should be on other archs as well, but we prefer performance over memory
# efficiency. Win64 is excluded because PIC is always on there and it
# complains when you give it the explicit flag.)
if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_SIZEOF_VOID_P MATCHES 8) AND NOT WIN32)
  set_target_properties(zlib PROPERTIES COMPILE_FLAGS -fPIC)
endif()
