dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([librfb], [1.5.0a1], [http://www.tightvnc.com/bugs.html])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_MAKE_SET
AC_LANG([C++])

case "`(uname -sr) 2>/dev/null`" in
"SunOS 5"*)
  SOLARIS=yes
  ;;
"Linux"*)
  LINUX=yes
  ;;
"IRIX 6"*)
  X_LIBS="-L/usr/lib32"
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
AC_CHECK_LIB(cl, clQuerySchemeFromName,
  [AC_DEFINE(HAVE_CL)
  PLATFORM_CXXSRCS="$PLATFORM_CXXSRCS IrixCLJpegCompressor.cxx"
  LIBS="$LIBS -lcl"])

dnl Checks for IRIX-specific Digital Media libraries.
AC_CHECK_LIB(dmedia, dmICCreate,
  [AC_DEFINE(HAVE_DMEDIA)
  PLATFORM_CXXSRCS="$PLATFORM_CXXSRCS IrixDMJpegCompressor.cxx"
  PLATFORM_CXXSRCS="$PLATFORM_CXXSRCS IrixDMIC_RawToJpeg.cxx"
  LIBS="$LIBS -ldmedia"])

AC_SUBST(PLATFORM_CXXSRCS)

AC_ARG_WITH([tight-zlib],
	AS_HELP_STRING([--with-tight-zlib],
		       [use libz which is distributed with VNC]),
	[], [with_tight_zlib='no'])

if test "x$with_tight_zlib" = xno; then
	AC_SEARCH_LIBS([inflateEnd], [z],
		[ZLIB_LIB='-lz'],
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

AC_ARG_WITH([tight-jpeg],
	AS_HELP_STRING([--with-tight-jpeg], 
		       [use libjpeg which is distributed with VNC]),
	[], [with_tight_jpeg='no'])

if test "x$with_tight_jpeg" = xno; then
  AC_SEARCH_LIBS([jpeg_destroy_compress], [jpeg],
	[JPEG_LIB='-ljpeg'],
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

AC_CHECK_FUNC(vsnprintf,VSNPRINTF_DEFINE='-DHAVE_VSNPRINTF',VSNPRINTF_DEFINE=)
AC_SUBST(VSNPRINTF_DEFINE)

AC_CHECK_FUNC(strcasecmp,STRCASECMP_DEFINE='-DHAVE_STRCASECMP',
  STRCASECMP_DEFINE=)
AC_SUBST(STRCASECMP_DEFINE)

AC_CHECK_FUNC(strncasecmp,STRNCASECMP_DEFINE='-DHAVE_STRNCASECMP',
  STRNCASECMP_DEFINE=)
AC_SUBST(STRNCASECMP_DEFINE)

AC_MSG_CHECKING(for socklen_t)
AC_TRY_COMPILE(
[#include <sys/types.h>
 #include <sys/socket.h>],
[socklen_t x;
accept(0, 0, &x);],
AC_MSG_RESULT(yes)
SOCKLEN_T_DEFINE='-DVNC_SOCKLEN_T=socklen_t',
AC_MSG_RESULT(using int)
SOCKLEN_T_DEFINE='-DVNC_SOCKLEN_T=int')
AC_SUBST(SOCKLEN_T_DEFINE)

BOILERPLATE=boilerplate.mk

if (sh -c "make --version" 2>/dev/null | grep GNU 2>&1 >/dev/null); then
  if sh -c "vncmkdepend" >/dev/null 2>&1; then
    BOILERPLATE="$BOILERPLATE:depend.mk"
  fi
fi

AC_OUTPUT(Makefile:Makefile.in:$BOILERPLATE \
         rdr/Makefile:rdr/Makefile.in:$BOILERPLATE \
         network/Makefile:network/Makefile.in:$BOILERPLATE \
         Xregion/Makefile:Xregion/Makefile.in:$BOILERPLATE \
         rfb/Makefile:rfb/Makefile.in:$BOILERPLATE \
)
