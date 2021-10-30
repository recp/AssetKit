#*****************************************************************************
# Copyright (C) 2020 Recep Aslantas                                          *
#                                                                            *
# Licensed under the Apache License, Version 2.0 (the "License");            *
# you may not use this file except in compliance with the License.           *
# You may obtain a copy of the License at                                    *
#                                                                            *
#      http://www.apache.org/licenses/LICENSE-2.0                            *
#                                                                            *
# Unless required by applicable law or agreed to in writing, software        *
# distributed under the License is distributed on an "AS IS" BASIS,          *
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
# See the License for the specific language governing permissions and        *
# limitations under the License.                                             *
#                                                                            *
#*****************************************************************************

AC_PREREQ([2.69])
AC_INIT([assetkit], [0.3.2], [info@recp.me])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Don't use the default cflags (-O2 -g), we set ours manually in Makefile.am.
: ${CFLAGS=""}

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])

CFLAGS="$CFLAGS -I./deps/xml/include"
CFLAGS="$CFLAGS -I./deps/cglm/include"
CFLAGS="$CFLAGS -I./deps/ds/include"
CFLAGS="$CFLAGS -I./deps/json/include"

LDFLAGS="$LDFLAGS -L./deps/ds/.libs"

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

AM_CONDITIONAL([MSYS],     [AS_CASE([$host_os],[msys*],         [true], [false])])
AM_CONDITIONAL([MINGW],    [AS_CASE([$host_os],[mingw*],        [true], [false])])

AC_CONFIG_FILES([makefile])

AC_OUTPUT
