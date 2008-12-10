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

if test "$GCC" = yes; then
  CFLAGS="$CFLAGS -Wall"
fi
if test "$GXX" = yes; then
  CXXFLAGS="$CXXFLAGS -Wall"
fi

CPPFLAGS="$CPPFLAGS -DWINVER=0x0500 -D_WIN32_IE=0x0500"
CXXFLAGS="$CXXFLAGS -mthreads"

AC_PATH_XTRA

AC_OUTPUT(Makefile
         vncviewer/Makefile
	 rfb_win32/Makefile
)
