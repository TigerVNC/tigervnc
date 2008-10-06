dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([tightvnc], [1.5.0a1], [http://www.tightvnc.com/bugs.html])
AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

AC_SUBST([COMMON_DIR], ['$(top_srcdir)/../common'])

AC_CONFIG_SUBDIRS([../common])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG([C++])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.14.6])

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

AC_ARG_ENABLE(vncconfig,
[  --enable-vncconfig      build the vncconfig utility (assumed by default),
                          use --disable-vncconfig to skip building vncconfig])
if test "$enable_vncconfig" = no; then
  echo "vncconfig utility will not be built"
  VNCCONFIG_DIR=
else
  VNCCONFIG_DIR='vncconfig'
fi
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

AC_OUTPUT(Makefile
         tx/Makefile
         x0vncserver/Makefile
         vncviewer/Makefile
         vncconfig/Makefile
         vncpasswd/Makefile
         po/Makefile.in
)
