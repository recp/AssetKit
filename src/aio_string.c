/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdlib.h>
#include <string.h>

#include "../include/assetio.h"
#include "aio_memory.h"

int
aio_strtof(char ** __restrict src,
           aio_float * __restrict dest,
           unsigned long n) {
  char * tok;

  dest = dest + n - 1ul;

  for (tok = strtok(*src, " ");
       tok && n > 0ul;
       tok = strtok(NULL, " "))
    *(dest - --n) = strtof(tok, NULL);

  return 0;
}

int
aio_strtof_nxn(char ** __restrict src,
               aio_float * __restrict dest,
               unsigned long m,
               unsigned long n) {
  char * tok;
  unsigned long idx;

  idx = m * n;
  dest = dest + idx - 1ul;

  for (tok = strtok(*src, " ");
       tok && idx > 0ul;
       tok = strtok(NULL, " "))
    *(dest - --idx) = strtof(tok, NULL);

  return 0;
}

int
aio_strtof_s(const char * __restrict src,
             aio_float * __restrict dest,
             unsigned long n) {
  char * raw;
  int    ret;

  raw = strdup(src);
  ret = aio_strtof(&raw, dest, n);
  free(raw);

  return ret;
}

inline
int
aio_strtof4(char ** __restrict src,
            aio_float4 * __restrict dest) {
  return aio_strtof(src, *dest, 4);
}

inline
int
aio_strtof4_s(const char * __restrict src,
              aio_float4 * __restrict dest) {
  return aio_strtof_s(src, *dest, 4);
}
