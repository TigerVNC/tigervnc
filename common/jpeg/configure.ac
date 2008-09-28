#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([libjpeg], [6.b])

AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

# Always build with prototypes
AC_DEFINE([HAVE_PROTOTYPES], 1, [Define if your compiler supports prototypes])
# Don't use undefined types
AC_DEFINE([INCOMPLETE_TYPES_BROKEN], 1, [Define if you want use complete types])

# Checks for programs.
AC_PROG_CC
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
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset memcpy], [],
	[AC_DEFINE([NEED_BSD_STRINGS], 1,
		   [Define if you have BSD-like bzero and bcopy])])

# config.h should not be included
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_HEADERS([jconfig.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
