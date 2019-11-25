#*****************************************************************************
# Copyright (c), Recep Aslantas.                                             *
#                                                                            *
# MIT License (MIT), http://opensource.org/licenses/MIT                      *
# Full license can be found in the LICENSE file                              *
#                                                                            *
#*****************************************************************************

AC_PREREQ([2.69])
AC_INIT([assetkit], [0.1.0], [info@recp.me])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])

CFLAGS="$CFLAGS -I./lib/libxml2/include"
CFLAGS="$CFLAGS -I./lib/jemalloc/include"
CFLAGS="$CFLAGS -I./lib/cglm/include"
CFLAGS="$CFLAGS -I./lib/curl/include"
CFLAGS="$CFLAGS -I./lib/libuv/include"
CFLAGS="$CFLAGS -I./lib/libds/include"
CFLAGS="$CFLAGS -I./lib/stb"
CFLAGS="$CFLAGS -I./lib/json/include"

LDFLAGS="$LDFLAGS -L./lib/jemalloc/lib"
LDFLAGS="$LDFLAGS -L./lib/libxml2/.libs"
LDFLAGS="$LDFLAGS -L./lib/curl/lib/.libs"
LDFLAGS="$LDFLAGS -L./lib/libuv/.libs"
LDFLAGS="$LDFLAGS -L./lib/libds/.libs"

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXX_C_O

AC_PROG_INSTALL
AM_PROG_AR

AC_ENABLE_SHARED
AC_ENABLE_STATIC

LT_INIT

# Checks for libraries.
AC_CHECK_LIB([m], [floor])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_SYS_LARGEFILE

# Checks for header files.
AC_CHECK_HEADERS([limits.h \
                  stddef.h \
                  stdint.h \
                  stdlib.h \
                  string.h \
                  wchar.h \
                  unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor \
                memmove \
                memset \
                strcasecmp \
                strchr \
                strdup \
                strrchr \
                strstr \
                strtol \
                strtoul])

AC_CONFIG_FILES([makefile])

AC_OUTPUT
