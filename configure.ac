dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.57])
AC_INIT([tigervnc], [0.0.91], [http://www.tigervnc.org])
AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

AC_CONFIG_SUBDIRS([common/jpeg common/zlib])

AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG([C++])
dnl AM_GNU_GETTEXT([external])
dnl AM_GNU_GETTEXT_VERSION([0.14.1])

case "`(uname -sr) 2>/dev/null`" in
"SunOS 5"*)
	SOLARIS=yes
	USE_MITSHM=yes
	USE_SUN_OVL=yes
	;;
"IRIX 6"*)
	LDFLAGS="-L/usr/lib32 $LDFLAGS"
	USE_MITSHM=yes
	USE_READDISPLAY=yes
	;;
"LynxOS 2"*)
	SJLJ_EXCEPTIONS=yes
	;;
esac

BUILD_WIN=
case "$host_os" in
mingw*)
	CPPFLAGS="$CPPFLAGS -DWINVER=0x0500 -D_WIN32_IE=0x0500"
	CXXFLAGS="$CXXFLAGS -mthreads"
	BUILD_WIN=yes
	AC_CHECK_TOOL([WINDRES], windres, [AC_MSG_ERROR(windres not found)])
	;;
esac
AM_CONDITIONAL([BUILD_WIN], [test "x$BUILD_WIN" = xyes])

dnl FIXME: Check for MIT-SHM properly, add a corresponding --with option.
if test "$USE_MITSHM" = yes; then
	MITSHM_DEFINE="-DHAVE_MITSHM"
fi
AC_SUBST(MITSHM_DEFINE)

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

AC_PATH_XTRA

VNCCONFIG_DIR='vncconfig'
AC_ARG_ENABLE([vncconfig],
	AS_HELP_STRING([--enable-vncconfig],
		       [build the vncconfig utility (default yes)]),
	[if test "$enableval" = no; then VNCCONFIG_DIR=; fi ], [])
AC_SUBST(VNCCONFIG_DIR)

dnl Check for the XTest X11 extension library.
AC_CHECK_LIB(Xtst,XTestGrabControl,USE_XTEST=yes,USE_XTEST=,
	     ["$X_LIBS" -lXext -lX11])
if test "$USE_XTEST" = yes; then
	XTEST_DEFINE='-DHAVE_XTEST'
	XTEST_LIB=-lXtst
else
	echo Warning: No XTest extension, building x0vncserver view-only
	XTEST_DEFINE=
	XTEST_LIB=
fi
AC_SUBST(XTEST_DEFINE)
AC_SUBST(XTEST_LIB)

dnl Support for READDISPLAY (Irix) and SUN_OVL (Solaris) extensions
dnl FIXME: Implement corresponding --with options.
if test "$USE_READDISPLAY" = yes; then
	READDISPLAY_DEFINE='-DHAVE_READDISPLAY'
elif test "$USE_SUN_OVL" = yes; then
	READDISPLAY_DEFINE='-DHAVE_SUN_OVL'
else
	READDISPLAY_DEFINE=
fi
AC_SUBST(READDISPLAY_DEFINE)

dnl Under Lynx/OS 2.3, we have to link with -lbsd to resolve
dnl gethostbyname, inet_addr, htons etc. Check if it's necessary.
dnl NOTE: Did not want to use AC_SEARCH_LIBS which would add
dnl   -lbsd to LIBS. We set INET_LIB instead.
AC_LANG_SAVE
AC_LANG_C
AC_CHECK_FUNC(gethostbyname,INET_LIB_REQ=,INET_LIB_REQ=yes)
if test "$INET_LIB_REQ" = yes; then
	AC_CHECK_LIB(bsd,gethostbyname,INET_LIB=-lbsd,INET_LIB=)
fi
AC_LANG_RESTORE
AC_SUBST(INET_LIB)

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
INCLUDED_JPEG=yes
AC_ARG_WITH([system-jpeg],
	AS_HELP_STRING([--with-system-jpeg], 
		       [use libjpeg which is distributed with the O/S]),
	[AC_SEARCH_LIBS([jpeg_destroy_compress], [jpeg],
			[INCLUDED_JPEG=no], [])],
	[])

AM_CONDITIONAL([INCLUDED_JPEG], [ test "x$INCLUDED_JPEG" = xyes ])
AC_CONFIG_SUBDIRS([jpeg])

AC_CHECK_FUNCS([vsnprintf snprintf strcasecmp strncasecmp])

# IPv6 related functions
AC_CHECK_FUNCS([inet_ntop getaddrinfo])

AC_CHECK_TYPES([socklen_t], [], [], [[#include <sys/socket.h>]])

AC_CHECK_HEADERS([sys/select.h])

AC_OUTPUT([
Makefile
common/Makefile
common/os/Makefile
common/rdr/Makefile
common/network/Makefile
common/Xregion/Makefile
common/rfb/Makefile
unix/Makefile
unix/tx/Makefile
unix/x0vncserver/Makefile
unix/vncviewer/Makefile
unix/vncconfig/Makefile
unix/vncpasswd/Makefile
win/Makefile
win/vncviewer/Makefile
win/rfb_win32/Makefile
])
