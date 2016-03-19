/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdlib.h>
#include <string.h>

#include "../include/assetkit.h"
#include "ak_memory.h"

AkResult
ak_strtof(char ** __restrict src,
           ak_float * __restrict dest,
           unsigned long n) {
  char * tok;

  dest = dest + n - 1ul;

  for (tok = strtok(*src, " ");
       tok && n > 0ul;
       tok = strtok(NULL, " "))
    *(dest - --n) = strtof(tok, NULL);

  return AK_OK;
}

AkResult
ak_strtomf(char ** __restrict src,
            ak_float * __restrict dest,
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

  return AK_OK;
}

AkResult
ak_strtomb(char ** __restrict src,
            ak_bool * __restrict dest,
            unsigned long m,
            unsigned long n) {
  char * tok;
  unsigned long idx;

  idx = m * n;
  dest = dest + idx - 1ul;

  for (tok = strtok(*src, " ");
       tok && idx > 0ul;
       tok = strtok(NULL, " "))
    *(dest - --idx) = (bool)strtol(tok, NULL, 10);

  return AK_OK;
}

AkResult
ak_strtomi(char ** __restrict src,
            ak_int * __restrict dest,
            unsigned long m,
            unsigned long n) {
  char * tok;
  unsigned long idx;

  idx = m * n;
  dest = dest + idx - 1ul;

  for (tok = strtok(*src, " ");
       tok && idx > 0ul;
       tok = strtok(NULL, " "))
    *(dest - --idx) = (ak_int)strtol(tok, NULL, 10);

  return AK_OK;
}

AkResult
ak_strtof_s(const char * __restrict src,
             ak_float * __restrict dest,
             unsigned long n) {
  char * raw;
  int    ret;

  raw = strdup(src);
  ret = ak_strtof(&raw, dest, n);
  free(raw);

  return AK_OK;
}

inline
AkResult
ak_strtof4(char ** __restrict src,
            ak_float4 * __restrict dest) {
  return ak_strtof(src, *dest, 4);
}

inline
AkResult
ak_strtof4_s(const char * __restrict src,
              ak_float4 * __restrict dest) {
  return ak_strtof_s(src, *dest, 4);
}
