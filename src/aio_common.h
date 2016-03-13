/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef aio_common_h
#define aio_common_h

#include "../include/assetio.h"
#include <stddef.h>
#include <sys/types.h>
#include <string.h>

#define AIO_IS_EQ_CASE(s1, s2) strcasecmp(s1, s2) == 0

#ifdef _MSC_VER
#  define strncasecmp _strnicmp
#  define strcasecmp _stricmp
#endif

#ifdef __GNUC__
#  define AIO_DESTRUCTOR __attribute__((destructor))
#  define AIO_CONSTRUCTOR __attribute__((constructor))
#else
#  define AIO_DESTRUCTOR
#  define AIO_CONSTRUCTOR
#endif

#define UNUSED(x) (void)(x)

typedef struct aio_enumpair_s aio_enumpair;
struct aio_enumpair_s {
  const char * key;
  long val;
};

int _assetio_hide
aio_enumpair_cmp(const void * a, const void * b);

int _assetio_hide
aio_enumpair_cmp2(const void * a, const void * b);

#endif /* aio_common_h */
