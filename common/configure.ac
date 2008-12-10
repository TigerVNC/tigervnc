dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([librfb], [1.5.0a1], [http://www.tightvnc.com/bugs.html])

AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG([C++])

AC_CONFIG_HEADERS([common-config.h])
CPPFLAGS="-DHAVE_COMMON_CONFIG_H $CPPFLAGS"

case "`(uname -sr) 2>/dev/null`" in
"SunOS 5"*)
  SOLARIS=yes
  ;;
"IRIX 6"*)
  LDFLAGS="-L/usr/lib32 $LDFLAGS"
  ;;
"LynxOS 2"*)
  SJLJ_EXCEPTIONS=yes
  ;;
esac

case "$host_os" in
    mingw*)
        CPPFLAGS="$CPPFLAGS -DWINVER=0x0500 -D_WIN32_IE=0x0500"
        CXXFLAGS="$CXXFLAGS -mthreads"
        ;;
esac

dnl FIXME: Remove duplication between this script and ../unix/configure.in
if test "$GCC" = yes; then
  CFLAGS="$CFLAGS -Wall"
  if test "$SOLARIS" = yes; then
    CFLAGS="$CFLAGS -Wno-unknown-pragmas -Wno-implicit-int"
  fi
fi
if test "$GXX" = yes; then
  CXXFLAGS="$CXXFLAGS -Wall"
  if test "$SOLARIS" = yes; then
    CXXFLAGS="$CXXFLAGS -Wno-unknown-pragmas -fpermissive"
  fi
  if test "$SJLJ_EXCEPTIONS" = yes; then
    CXXFLAGS="$CXXFLAGS -fsjlj-exceptions"
  fi
fi

dnl Checks for IRIX-specific Compression Library.
AC_SEARCH_LIBS([clQuerySchemeFromName])
AC_CHECK_LIB(cl, clQuerySchemeFromName,
  [AC_DEFINE([HAVE_CL], 1, [Define if you have IRIX compression library])
  IRIX_COMPRESS=yes])
AM_CONDITIONAL([IRIX_COMPRESS], [ test "x$IRIX_COMPRESS" = xyes ])

dnl Checks for IRIX-specific Digital Media libraries.
AC_CHECK_LIB(dmedia, dmICCreate,
  [AC_DEFINE(HAVE_DMEDIA, 1, [Define if you have IRIX Digital Media library])
  IRIX_MEDIA=yes])
AM_CONDITIONAL([IRIX_MEDIA], [ test "x$IRIX_MEDIA" = xyes ])

dnl Check for zlib library
INCLUDED_ZLIB=no
AC_ARG_WITH([included-zlib],
	AS_HELP_STRING([--with-included-zlib],
		       [use libz which is distributed with VNC]),
	[INCLUDED_ZLIB=yes],
	[AC_SEARCH_LIBS([inflateEnd], [z], [], [INCLUDED_ZLIB=yes])])

AM_CONDITIONAL([INCLUDED_ZLIB], [ test "x$INCLUDED_ZLIB" = xyes ])
AC_CONFIG_SUBDIRS([zlib])

dnl Check for libjpeg library
INCLUDED_JPEG=no
AC_ARG_WITH([included-jpeg],
	AS_HELP_STRING([--with-included-jpeg], 
		       [use libjpeg which is distributed with VNC]),
	[INCLUDED_JPEG=yes],
	[AC_SEARCH_LIBS([jpeg_destroy_compress], [jpeg], [],
			[INCLUDED_JPEG=yes])])

AM_CONDITIONAL([INCLUDED_JPEG], [ test "x$INCLUDED_JPEG" = xyes ])
AC_CONFIG_SUBDIRS([jpeg])

AC_CHECK_FUNCS_ONCE([vsnprintf snprintf strcasecmp strncasecmp])

# IPv6 related functions
AC_CHECK_FUNCS_ONCE([inet_ntop getaddrinfo])

AC_CHECK_TYPES([socklen_t])

AC_CHECK_HEADERS([sys/select.h])

AC_OUTPUT([Makefile
	   os/Makefile
	   rdr/Makefile
	   network/Makefile
	   Xregion/Makefile
	   rfb/Makefile])
