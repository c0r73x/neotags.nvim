dnl Process this file with autoconf to produce a configure script.

dnl AC_INIT(tree.c)
dnl dnl Every other copy of the package version number gets its value from here
dnl AM_INIT_AUTOMAKE(tree, 0.1.0)
 
AC_PREREQ([2.69])
AC_INIT([neotags],[0.5.0],[leason@ualberta.ca])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)
AC_SUBST(VERSION)
AC_CANONICAL_HOST
AC_GNU_SOURCE
AC_PREFIX_DEFAULT(["$HOME/.vim_tags"])

CFLAGS=['-march=native -s -Ofast -Wall -Wextra -Wpedantic']

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([bsd], [strlcpy])
AC_CHECK_LIB([pthread], [pthread_create])
PKG_CHECK_MODULES([PCRE2], [libpcre2-8])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDC
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strchr strdup strstr strtol])

AM_CONDITIONAL([NEED_BSD_FUNCS], [test "x$ac_cv_func_strlcpy" = "xno"])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
