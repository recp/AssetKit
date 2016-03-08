/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef aio_types_h
#define aio_types_h

#include "../include/assetio.h"
#include <stddef.h>
#include <sys/types.h>
#include <string.h>

#define aio_true  0x1
#define aio_false 0x0

#define AIO_IS_EQ_CASE(s1, s2) strcasecmp(s1, s2) == 0

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#define UNUSED(x) (void)(x)

typedef struct aio_enumpair_s aio_enumpair;
struct aio_enumpair_s {
  const char * key;
  long val;
};

static
inline
int _assetio_hide
aio_enumpair_cmp(const void * a, const void * b) {
  return strcmp(((const aio_enumpair *)a)->key,
                ((const aio_enumpair *)b)->key);
}

static
inline
int _assetio_hide
aio_enumpair_cmp2(const void * a, const void * b) {
  return strcmp(((const aio_enumpair *)b)->key, a);
}

#endif /* aio_types_h */
