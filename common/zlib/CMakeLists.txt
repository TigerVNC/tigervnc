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
