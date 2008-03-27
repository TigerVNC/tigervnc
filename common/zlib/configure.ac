#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([zlib], [1.1.4])
AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_CHECK_HEADERS([unistd.h errno.h])

# Checks for library functions.
AC_FUNC_MMAP

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
