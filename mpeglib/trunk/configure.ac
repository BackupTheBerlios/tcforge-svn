#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(mpeglib, 0.3.0, fromani@gmail.com)

AC_CONFIG_AUX_DIR([config])

AC_CONFIG_SRCDIR([mpeglib/mpeglib.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_DEFINE(_GNU_SOURCE, 1, [GNU C extensions])

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_SYS_LARGEFILE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset strdup])

AC_CONFIG_FILES([Makefile
                 mpeglib/Makefile])
AC_OUTPUT
