dnl Process this file with autoconf to produce a configure script.

AC_INIT([tightvnc], [1.5.0a1], [http://www.tightvnc.com/bugs.html])
AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

dnl Checks for programs.
dnl AM_GNU_GETTEXT([external])
dnl AM_GNU_GETTEXT_VERSION([0.17])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_LANG([C++])

saved_cflags="$CFLAGS"
saved_cxxflags="$CXXFLAGS"
CFLAGS="$CFLAGS -Wall -Wextra"
CXXFLAGS="$CXXFLAGS -Wall -Wextra"

AC_MSG_CHECKING([if compiler supports -Wall and -Werror flags])
AC_COMPILE_IFELSE(
 [AC_LANG_PROGRAM([])],
 [AC_MSG_RESULT([yes])],
 [AC_MSG_RESULT([no])
  CFLAGS="$saved_cflags"
  CXXFLAGS="$saved_cxxflags"])

AC_SUBST([COMMON_DIR], ['$(top_srcdir)/common'])
AC_SUBST([UNIX_DIR], ['$(top_srcdir)/unix'])
AC_SUBST([TX_DIR], ['$(UNIX_DIR)/tx'])
AC_SUBST([INTL_DIR], ['$(UNIX_DIR)/intl'])
AC_SUBST([RFB_LIBS], ['$(COMMON_DIR)/rfb/librfb.la'])
AC_SUBST([RDR_LIBS], ['$(COMMON_DIR)/rdr/librdr.la'])
AC_SUBST([NETWORK_LIBS], ['$(COMMON_DIR)/network/libnetwork.la'])
AC_SUBST([TX_LIBS], ['$(TX_DIR)/libtx.la'])

AC_CONFIG_HEADERS([config.h])

dnl XXX this statement really bother me
case "`(uname -sr) 2>/dev/null`" in
"SunOS 5"*)
  SOLARIS=yes
  USE_MITSHM=yes
  USE_SUN_OVL=yes
  ;;
"Linux"*)
  LINUX=yes
  USE_MITSHM=yes
  ;;
"IRIX 6"*)
  X_LIBS="-L/usr/lib32"
  USE_MITSHM=yes
  USE_READDISPLAY=yes
  ;;
"LynxOS 2"*)
  SJLJ_EXCEPTIONS=yes
  ;;
esac

dnl FIXME: Check for MIT-SHM properly, add a corresponding --with option.
if test "$USE_MITSHM" = yes; then
	MITSHM_DEFINE="-DHAVE_MITSHM"
fi
AC_SUBST(MITSHM_DEFINE)

if test "$GCC" = yes; then
	if test "$SOLARIS" = yes; then
		CFLAGS="$CFLAGS -Wno-unknown-pragmas -Wno-implicit-int"
	fi
fi
if test "$GXX" = yes; then
	if test "$SOLARIS" = yes; then
		CXXFLAGS="$CXXFLAGS -Wno-unknown-pragmas -fpermissive"
	fi
	if test "$SJLJ_EXCEPTIONS" = yes; then
		CXXFLAGS="$CXXFLAGS -fsjlj-exceptions"
	fi
fi

AC_PATH_XTRA

AC_ARG_ENABLE([vncconfig],
	AS_HELP_STRING([--enable-vncconfig],
		       [build the vncconfig utility(default yes)]),
	[BUILD_VNCCONFIG="$enableval"], [BUILD_VNCCONFIG="yes"])
AM_CONDITIONAL([VNCCONFIG], [test "x$BUILD_VNCCONFIG" = xyes])

dnl Check for the XTest X11 extension library.
AC_SEARCH_LIBS([XTestGrabControl], [Xtst],
	[AC_DEFINE([HAVE_XTEST], [], [Define if you have Xtst library])],
	[AC_MSG_WARN([No XTest extension, building x0vncserver view-only])])

dnl Support for READDISPLAY (Irix) and SUN_OVL (Solaris) extensions
dnl FIXME: Implement corresponding --with options.
if test "$USE_READDISPLAY" = yes; then
  AC_DEFINE([HAVE_READDISPLAY], [], [Define if you have READDISPLAY extension])
elif test "$USE_SUN_OVL" = yes; then
  AC_DEFINE([HAVE_SUN_OVL], [], [Define if you have SUN_OVL extension])
fi

dnl Under Lynx/OS 2.3, we have to link with -lbsd to resolve
dnl gethostbyname, inet_addr, htons etc. Check if it's necessary.
AC_SEARCH_LIBS([gethostbyname], [c bsd])


dnl Checks for IRIX-specific Compression Library.
AC_SEARCH_LIBS([clQuerySchemeFromName], [cl],
	[AC_DEFINE([HAVE_CL], [], [Define if you have cl library])
	 USE_CL=yes])
AM_CONDITIONAL([CL], [test "x$USE_CL" = xyes])

dnl Checks for IRIX-specific Digital Media libraries.
AC_SEARCH_LIBS([dmICCreate], [dmedia],
	[AC_DEFINE([HAVE_DMEDIA], [], [Define if you have dmedia library])
	 USE_DMEDIA=yes])
AM_CONDITIONAL([DMEDIA], [test "x$USE_DMEDIA" = xyes])

AC_SEARCH_LIBS([inflate], [z], [],
	[AC_CONFIG_SUBDIRS([common/zlib])
	 ZLIB_LIBS='$(COMMON_DIR)/zlib/libz.la'
	 VNC_ZLIB=yes])
AM_CONDITIONAL([ZLIB], [test "x$VNC_ZLIB" = xyes])
AC_SUBST(ZLIB_LIBS)

AC_SEARCH_LIBS([jpeg_destroy_compress], [jpeg], [],
dnl	[AC_CONFIG_SUBDIRS([common/jpeg])
	[JPEG_LIBS='$(COMMON_DIR)/jpeg/libjpeg.a'
	 VNC_JPEG=yes])
AM_CONDITIONAL([JPEG], [test "x$VNC_JPEG" = xyes])
AC_SUBST(JPEG_LIBS)

AC_CHECK_FUNCS_ONCE([vsnprintf strcasecmp strncasecmp])

AC_MSG_CHECKING([for socklen_t])
AC_COMPILE_IFELSE(
 [AC_LANG_PROGRAM(
  [[#include <sys/types.h>
    #include <sys/socket.h>]],
  [[socklen_t x;
    accept(0, 0, &x);]])],
 [AC_MSG_RESULT([yes])
  AC_DEFINE([VNC_SOCKLEN_T], [socklen_t], [Define to socklen_t or int])],
 [AC_MSG_RESULT([using int])
  AC_DEFINE([VNC_SOCKLEN_T], [int], [Define to socklen_t or int])])

AC_CONFIG_FILES([Makefile
	   common/Makefile
	   common/rdr/Makefile
	   common/network/Makefile
	   common/Xregion/Makefile
	   common/rfb/Makefile
	   unix/Makefile
	   unix/tx/Makefile
	   unix/x0vncserver/Makefile
	   unix/vncviewer/Makefile
	   unix/vncconfig/Makefile
	   unix/vncpasswd/Makefile])
AC_OUTPUT
