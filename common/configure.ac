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

AC_ARG_WITH([included-zlib],
	AS_HELP_STRING([--with-included-zlib],
		       [use libz which is distributed with VNC]),
	[], [with_included_zlib='no'])

if test "x$with_included_zlib" = xno; then
	AC_SEARCH_LIBS([inflateEnd], [z], [],
		[ZLIB_DIR=zlib
		 ZLIB_INCLUDE='-I$(top_srcdir)/zlib'
		 ZLIB_LIB='$(top_srcdir)/zlib/libz.la'])
else
	ZLIB_DIR=zlib
	ZLIB_INCLUDE='-I$(top_srcdir)/zlib'
	ZLIB_LIB='$(top_srcdir)/zlib/libz.la'
fi

AC_SUBST(ZLIB_DIR)
AC_SUBST(ZLIB_INCLUDE)
AC_SUBST(ZLIB_LIB)

AC_CONFIG_SUBDIRS([zlib])

AC_ARG_WITH([included-jpeg],
	AS_HELP_STRING([--with-included-jpeg], 
		       [use libjpeg which is distributed with VNC]),
	[], [with_included_jpeg='no'])

if test "x$with_included_jpeg" = xno; then
  AC_SEARCH_LIBS([jpeg_destroy_compress], [jpeg], [],
	[JPEG_DIR=jpeg
	 JPEG_INCLUDE='-I$(top_srcdir)/jpeg'
	 JPEG_LIB='$(top_srcdir)/jpeg/libjpeg.la'])
else
	JPEG_DIR=jpeg
	JPEG_INCLUDE='-I$(top_srcdir)/jpeg'
	JPEG_LIB='$(top_srcdir)/jpeg/libjpeg.la'
fi

AC_CONFIG_SUBDIRS([jpeg])

AC_SUBST(JPEG_DIR)
AC_SUBST(JPEG_INCLUDE)
AC_SUBST(JPEG_LIB)

AC_CHECK_FUNCS_ONCE([vsnprintf strcasecmp strncasecmp])

AC_CHECK_TYPES([socklen_t],
	[AC_DEFINE([VNC_SOCKLEN_T], [socklen_t], [Use correct size])],
	[AC_DEFINE([VNC_SOCKLEN_T], [int])])

AC_OUTPUT([Makefile
	   rdr/Makefile
	   network/Makefile
	   Xregion/Makefile
	   rfb/Makefile])
