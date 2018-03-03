/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/assetkit.h"

#define AK_ARRAY_SEP_CHECK (c == ' ' || c == '\n' || c == '\t' || c == '\r')

AK_EXPORT
const char*
ak_strltrim_fast(const char * __restrict str) {
  const char *ptr;
  size_t      len, i;
  char        c;

  len = strlen(str);
  ptr = str;

  if (len == 0)
    return ptr;

  for (i = 0; i < len; i++) {
    c = str[i];
    if (AK_ARRAY_SEP_CHECK) {
      ptr++;
      continue;
    } else {
      return ptr;
    }
  }

  return ptr;
}

AK_EXPORT
int
ak_strtok_count(char * __restrict buff,
                char * __restrict sep,
                size_t           *len) {
  int i, count, itemc, buflen, found_sep;

  buflen = (int)strlen(buff);
  if (buflen == 0)
    return 0;

  count = itemc = 0;

  /* because of buff[j + 1] */
  if (buflen == 1)
    return 1;

  found_sep = false;
  for (i = 0; i < buflen; i++) {
    if (strchr(sep, buff[i])){
      if (!found_sep) {
        itemc++;
        found_sep = true;
      }
      continue;
    }

    found_sep = false;
    count++;
  }

  if (len)
    *len = buflen - count;

  /* left trim */
  if (strchr(sep, buff[0]))
    itemc--;

  /* right trim */
  if (strchr(sep, buff[buflen - 1]))
    itemc--;

  return itemc + 1;
}

AK_EXPORT
int
ak_strtok_count_fast(char * __restrict buff,
                     size_t           *len) {
  int i, count, itemc, buflen, found_sep;
  char c;

  buflen = (int)strlen(buff);
  if (buflen == 0)
    return 0;

  count = itemc = 0;

  /* because of buff[j + 1] */
  if (buflen == 1)
    return 1;

  found_sep = false;
  for (i = 0; i < buflen; i++) {
    c = buff[i];
    if (AK_ARRAY_SEP_CHECK) {
      if (!found_sep) {
        itemc++;
        found_sep = true;
      }
      continue;
    }

    found_sep = false;
    count++;
  }

  /* left trim */
  c = buff[0];
  if (AK_ARRAY_SEP_CHECK)
    itemc--;

  /* right trim */
  c = buff[buflen - 1];
  if (AK_ARRAY_SEP_CHECK)
    itemc--;

  if (len)
    *len = buflen - count;
  
  return itemc + 1;
}


AK_EXPORT
int
ak_strtof_fast(char    * __restrict src,
               AkFloat * __restrict dest,
               unsigned long n) {
  char *tok, *end;
  char  c;

  if (n == 0)
    return 0;

  dest = dest + n - 1ul;

  tok = src;
  while ((void)(c = *tok), AK_ARRAY_SEP_CHECK)
    tok++;

  while (n > 1ul) {
    *(dest - --n) = strtof(tok, &end);
    tok = end;

    while ((void)(c = *tok), AK_ARRAY_SEP_CHECK)
      tok++;
  }

  *(dest - --n) = strtof(tok, NULL);

  return (int)n;
}

AK_EXPORT
int
ak_strtod_fast(char     * __restrict src,
               AkDouble * __restrict dest,
               unsigned long n) {
  char *tok, *end;
  char  c;

  if (n == 0)
    return 0;

  dest = dest + n - 1ul;

  tok = src;
  while ((void)(c = *tok), AK_ARRAY_SEP_CHECK)
    tok++;

  while (n > 1ul) {
    *(dest - --n) = strtod(tok, &end);
    tok = end;

    while ((void)(c = *tok), AK_ARRAY_SEP_CHECK)
      tok++;
  }

  *(dest - --n) = strtod(tok, NULL);

  return (int)n;
}

AK_EXPORT
int
ak_strtoui_fast(char   * __restrict src,
                AkUInt * __restrict dest,
                unsigned long n) {
  char *tok, *end;
  char  c;

  if (n == 0)
    return 0;

  dest = dest + n - 1ul;

  tok = src;
  while ((void)(c = *tok), AK_ARRAY_SEP_CHECK)
    tok++;

  while (n > 1ul) {
    *(dest - --n) = (AkUInt)strtoul(tok, &end, 10);
    tok = end;

    while ((void)(c = *tok), AK_ARRAY_SEP_CHECK)
      tok++;
  }

  *(dest - --n) = (AkUInt)strtoul(tok, NULL, 10);

  return (int)n;
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
ak_strtoui(char   ** __restrict src,
           AkUInt * __restrict dest,
           unsigned long n) {
  char *tok;

  dest = dest + n - 1ul;

  for (tok = strtok(*src, " \t\r\n");
       tok && n > 0ul;
       tok = strtok(NULL, " \t\r\n"))
    *(dest - --n) = (AkUInt)strtoul(tok, NULL, 10);

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

AK_EXPORT
char*
ak_tolower(char *str) {
  char *p;
  for (p = str; *p; ++p) *p = tolower(*p);
  return str;
}

AK_EXPORT
char*
ak_toupper(char *str) {
  char *p;
  for (p = str; *p; ++p) *p = toupper(*p);
  return str;
}
