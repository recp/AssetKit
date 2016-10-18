/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdlib.h>
#include <string.h>

#include "../include/assetkit.h"

AK_EXPORT
AkResult
ak_strtod(char ** __restrict src,
          AkDouble * __restrict dest,
          unsigned long n) {
  char * tok;

  dest = dest + n - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && n > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --n) = strtod(tok, NULL);

  return AK_OK;
}

AK_EXPORT
AkResult
ak_strtof(char ** __restrict src,
           AkFloat * __restrict dest,
           unsigned long n) {
  char * tok;

  dest = dest + n - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && n > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --n) = strtof(tok, NULL);

  return AK_OK;
}

AK_EXPORT
AkResult
ak_strtomf(char ** __restrict src,
            AkFloat * __restrict dest,
            unsigned long m,
            unsigned long n) {
  char * tok;
  unsigned long idx;

  idx = m * n;
  dest = dest + idx - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && idx > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --idx) = strtof(tok, NULL);

  return AK_OK;
}

AK_EXPORT
AkResult
ak_strtomb(char ** __restrict src,
            AkBool * __restrict dest,
            unsigned long m,
            unsigned long n) {
  char * tok;
  unsigned long idx;

  idx = m * n;
  dest = dest + idx - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && idx > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --idx) = (bool)strtol(tok, NULL, 10);

  return AK_OK;
}

AK_EXPORT
AkResult
ak_strtomi(char ** __restrict src,
            AkInt * __restrict dest,
            unsigned long m,
            unsigned long n) {
  char * tok;
  unsigned long idx;

  idx = m * n;
  dest = dest + idx - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && idx > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --idx) = (AkInt)strtol(tok, NULL, 10);

  return AK_OK;
}

AK_EXPORT
AkResult
ak_strtof_s(const char * __restrict src,
             AkFloat * __restrict dest,
             unsigned long n) {
  char * raw;
  AkResult ret;

  raw = strdup(src);
  ret = ak_strtof(&raw, dest, n);
  free(raw);

  return ret;
}

AK_EXPORT
inline
AkResult
ak_strtof4(char ** __restrict src,
            AkFloat4 * __restrict dest) {
  return ak_strtof(src, *dest, 4);
}

AK_EXPORT
inline
AkResult
ak_strtof4_s(const char * __restrict src,
              AkFloat4 * __restrict dest) {
  return ak_strtof_s(src, *dest, 4);
}
