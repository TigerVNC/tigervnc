dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.57])

dnl Modify also RCVERSION variable when modifying release
AC_INIT([tigervnc], [1.0.90], [http://www.tigervnc.org])
RCVERSION="1,0,90,0"
AC_SUBST([RCVERSION])

BUILD=`date +%Y%m%d`
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

AC_CONFIG_SUBDIRS([common/jpeg common/zlib])

AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG([C++])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.14.1])

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
mingw32*)
	CPPFLAGS="$CPPFLAGS -DWINVER=0x0500 -D_WIN32_IE=0x0500"
	CXXFLAGS="$CXXFLAGS -mthreads"
	BUILD_WIN=yes
	AC_CHECK_TOOL([WINDRES], windres, [AC_MSG_ERROR(windres not found)])
	;;
mingw*)
	AC_MSG_ERROR(["Only 32bit MinGW builds are supported"])
	;;
esac
AM_CONDITIONAL([BUILD_WIN], [test "x$BUILD_WIN" = xyes])
if test "x$BUILD_WIN" = xyes; then
	AC_DEFINE([BUILD_WIN],1,[is it a windows build])
fi

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

dnl Check gnutls availability
AC_ARG_ENABLE([gnutls],
	AS_HELP_STRING([--enable-gnutls],
		       [build in GNUTLS based encryption support (default yes)]),
	[enable_gnutls="$enableval"], [enable_gnutls=yes])
AC_ARG_VAR(GNUTLS_CFLAGS, [Custom C compiler flags for using GnuTLS, e.g. -I{GnuTLS directory}/include])
AC_ARG_VAR(GNUTLS_LDFLAGS, [Custom linker flags for using GnuTLS, e.g. -L{GnuTLS directory}/lib -lgnutls])

GNUTLS_LIBS=
if test "x$enable_gnutls" = xyes; then
	if test "x$GNUTLS_LDFLAGS" = "x"; then
		GNUTLS_LDFLAGS='-lgnutls -lgcrypt -lgpg-error'
		AC_CHECK_LIB([tasn1], [asn1_create_element], [GNUTLS_LDFLAGS="${GNUTLS_LDFLAGS} -ltasn1"])
	fi
	SAVE_LIBS=${LIBS}
	LIBS="${LIBS} ${GNUTLS_LDFLAGS} -lz"
	SAVE_CPPFLAGS=${CPPFLAGS}
	CPPFLAGS="${CPPFLAGS} ${GNUTLS_CFLAGS}"
	AC_MSG_CHECKING([for GnuTLS library])
	AC_LINK_IFELSE(AC_LANG_CALL([], gnutls_global_init),
		       [GNUTLS_LIBS=${GNUTLS_LDFLAGS}
			AC_DEFINE(HAVE_GNUTLS, 1, [Is gnutls present? ])
			AC_MSG_RESULT(yes)], AC_MSG_RESULT(no))
	AC_MSG_CHECKING([for gnutls_transport_set_global_errno() function])
	AC_LINK_IFELSE(AC_LANG_CALL([], gnutls_transport_set_global_errno),
		       AC_MSG_RESULT(yes),
		       [AC_DEFINE(HAVE_OLD_GNUTLS, 1, [Does gnutls lack the gnutls_transport_set_global_errno() function? ])
			AC_MSG_RESULT(no)])
	AC_MSG_CHECKING([for gnutls_x509_crt_print() function])
	AC_LINK_IFELSE(AC_LANG_CALL([], gnutls_x509_crt_print),
		       [AC_DEFINE(HAVE_GNUTLS_X509_CRT_PRINT, 1, [Is the gnutls_x509_crt_print() function present? ])
			AC_MSG_RESULT(yes)],
			AC_MSG_RESULT(no))
	AC_CHECK_TYPES([gnutls_x509_crt_t, gnutls_datum_t,
			gnutls_pk_algorithm_t, gnutls_sign_algorithm_t],
			[], [], [#include <gnutls/gnutls.h>])
	LIBS=${SAVE_LIBS}
fi
AC_SUBST([GNUTLS_LIBS])
AM_CONDITIONAL([HAVE_GNUTLS], [ ! test "x$GNUTLS_LIBS" = x ])

AC_ARG_ENABLE([pam],
	AS_HELP_STRING([--enable-pam],
		       [build in support for authenticating users using PAM (default yes)]),
	[enable_pam="$enableval"], [enable_pam=yes])

PAM_LIBS=
if test "x$enable_pam" = xyes; then
	AC_CHECK_LIB([pam], [pam_start], 
		     [AC_CHECK_HEADER([security/pam_appl.h],
				     [PAM_LIBS='-lpam'
				      AC_DEFINE(HAVE_PAM, 1, [PAM available])],
				     [], [#include <stdio.h>])])
fi
AC_SUBST([PAM_LIBS])
AM_CONDITIONAL([HAVE_PAM], [ ! test "x$PAM_LIBS" = x ])

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

AC_CHECK_FUNC(setenv, AC_DEFINE([HAVE_SETENV], [1], [Is setenv() present?]))

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

# Network functions
AC_SEARCH_LIBS([inet_aton], [resolv])
AC_CHECK_FUNCS([inet_aton])

# IPv6 related functions
AC_CHECK_FUNCS([inet_ntop getaddrinfo])

AC_CHECK_TYPES([socklen_t], [], [], [[#include <sys/socket.h>]])

AC_CHECK_HEADERS([sys/select.h])

# Needed for WinVNC, but not part of mingw yet
AC_CHECK_DECLS([CLSID_ActiveDesktop], [BUILD_WINVNC=yes], [],
	       [[#include <windows.h>
	         #include <shlguid.h>
	         ]])
AM_CONDITIONAL([BUILD_WINVNC], [ test "x$BUILD_WINVNC" = xyes ])

# Packaging stuff
AM_CONDITIONAL([X86_64], [test "x$host_cpu" = "xx86_64"])

AC_SUBST(BUILD)

AC_CONFIG_FILES([makemacpkg:release/makemacpkg.in])
AC_CONFIG_FILES([maketarball:release/maketarball.in])

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
win/vncconfig/Makefile
win/winvnc/Makefile
win/resdefs.h
po/Makefile.in
])
