#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.57])
AC_INIT([libjpeg], [6.b])

AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

# Always build with prototypes
AC_DEFINE([HAVE_PROTOTYPES], 1, [Define if your compiler supports prototypes])
# Don't use undefined types
AC_DEFINE([INCOMPLETE_TYPES_BROKEN], 1, [Define if you want use complete types])

# Checks for programs.
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdlib.h string.h])
AC_CHECK_HEADER([sys/types.h], AC_DEFINE([NEED_SYS_TYPES_H], 1, [Define if you have sys/types.h]))

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_CHAR_UNSIGNED
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([unsigned char, unsigned short])

AC_MSG_CHECKING([if right shift is signed])
AC_TRY_RUN(
	[#include <stdio.h>
	 int is_shifting_signed (long arg) {
	 long res = arg >> 4;

	 if (res == -0x7F7E80CL)
		return 1; /* right shift is signed */

	 /* see if unsigned-shift hack will fix it. */
	 /* we can't just test exact value since it depends on width of long... */
	 res |= (~0L) << (32-4);
	 if (res == -0x7F7E80CL)
		return 0; /* right shift is unsigned */

	 printf("Right shift isn't acting as I expect it to.\n");
	 printf("I fear the JPEG software will not work at all.\n\n");
	 return 0; /* try it with unsigned anyway */
	 }
	 int main (void) {
		exit(is_shifting_signed(-0x7F7E80B1L));
	 }],
	[AC_MSG_RESULT(no)
	 AC_DEFINE([RIGHT_SHIFT_IS_UNSIGNED], 1, [Define if shift is unsigned])],
	[AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(Assuming that right shift is signed on target machine.)])

# test whether global names are unique to at least 15 chars
AC_MSG_CHECKING([for short external names])
AC_TRY_LINK(
	[int possibly_duplicate_function () { return 0; }
	 int possibly_dupli_function () { return 1; }], [ ],
	[AC_MSG_RESULT(ok)],
	[AC_MSG_RESULT(short)
	 AC_DEFINE([NEED_SHORT_EXTERNAL_NAMES], 1, [Define if you need short function names])])

# Checks for library functions.
AC_CHECK_FUNCS([memset memcpy], [],
	[AC_DEFINE([NEED_BSD_STRINGS], 1,
		   [Define if you have BSD-like bzero and bcopy])])

# Set flags to indicate platform
case "$host_os" in
  cygwin* | mingw* | pw32* | interix*)
    is_win32=1
  ;;
esac
AM_CONDITIONAL([IS_WIN32], [test "x$is_win32" = "x1"])

# SIMD is optional
AC_ARG_WITH([simd],
    AC_HELP_STRING([--without-simd],[Omit accelerated SIMD routines.]))
if test "x${with_simd}" != "xno"; then
  # Check if we're on a supported CPU
  AC_MSG_CHECKING([if we have SIMD optimisations for cpu type])
  case "$host_cpu" in
    x86_64)
      AC_MSG_RESULT([yes (x86_64)])
      AC_PROG_NASM
      simd_arch=x86_64
    ;;
    i*86 | x86 | ia32)
      AC_MSG_RESULT([yes (i386)])
      AC_PROG_NASM
      simd_arch=i386
    ;;
    *)
      AC_MSG_RESULT([no ("$host_cpu")])
      with_simd=no
    ;;
  esac

  if test "x${with_simd}" != "xno"; then
    AC_DEFINE([WITH_SIMD], [1], [Use accelerated SIMD routines.])
  fi
fi

AM_CONDITIONAL([WITH_SIMD], [test "x$with_simd" != "xno"])
AM_CONDITIONAL([SIMD_I386], [test "x$simd_arch" = "xi386"])
AM_CONDITIONAL([SIMD_X86_64], [test "x$simd_arch" = "xx86_64"])

# jconfig.h is the file we use, but we have another before that to
# fool autoheader. the reason is that we include this header in our
# API headers, which can screw things up for users of the lib.
# jconfig.h is a minimal version that allows this package to be built
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_HEADERS([jconfig.h])
AC_CONFIG_FILES([Makefile simd/Makefile])
AC_OUTPUT
