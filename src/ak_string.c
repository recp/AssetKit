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
size_t
ak_strtok_count(char * __restrict buff,
                char * __restrict sep,
                size_t           *len) {
  size_t c, spacec, sepc, i;

  sepc = (uint32_t)strlen(sep);
  if (!sepc)
    return 1;

  c = spacec = 0;
  while(*buff != '\0') {
    bool sep_found, inc;

    inc = sep_found = false;
    do {
      sep_found = false;
      for (i = 0; i < sepc; i++) {
        if (*buff == sep[i]) {
          sep_found = true;
          inc       = c > 0; /* trim left */
          buff++;
        }
      }
    } while (sep_found && *buff != '\0');

    if (*buff == '\0')
      break;

    buff++;
    if (inc)
      spacec++;
    c++;
  }

  if (len)
    *len = c;

  return spacec + 1;
}

AK_EXPORT
int
ak_strtod(char ** __restrict src,
          AkDouble * __restrict dest,
          unsigned long n) {
  char *tok;

  dest = dest + n - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && n > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --n) = strtod(tok, NULL);

  return (int)n;
}

AK_EXPORT
int
ak_strtof(char ** __restrict src,
           AkFloat * __restrict dest,
           unsigned long n) {
  char *tok;

  dest = dest + n - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && n > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --n) = strtof(tok, NULL);

  return (int)n;
}

AK_EXPORT
int
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

  return (int)idx;
}

AK_EXPORT
int
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

  return (int)idx;
}

AK_EXPORT
int
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

  return (int)idx;
}

AK_EXPORT
int
ak_strtof_s(const char * __restrict src,
             AkFloat * __restrict dest,
             unsigned long n) {
  char *raw;
  int   ret;

  raw = strdup(src);
  ret = ak_strtof(&raw, dest, n);
  free(raw);

  return ret;
}

AK_EXPORT
inline
int
ak_strtof4(char ** __restrict src,
            AkFloat4 * __restrict dest) {
  return ak_strtof(src, *dest, 4);
}

AK_EXPORT
inline
int
ak_strtof4_s(const char * __restrict src,
              AkFloat4 * __restrict dest) {
  return ak_strtof_s(src, *dest, 4);
}
