/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/ak/assetkit.h"
#include "common.h"
#include "simd/scan.h"

static inline
bool
ak_str_isdigit(char c) {
  return c >= '0' && c <= '9';
}

static inline
bool
ak_str_sep(char c) {
  return c == ' ' || c == '\n' || c == '\t'
         || c == '\r' || c == '\f' || c == '\v';
}

static inline
bool
ak_str_sepline(char c) {
  return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}

static inline
char*
ak_str_skip_sep(char * __restrict p,
                char * __restrict end,
                bool              lineOnly) {
  if (end) {
    char *shortEnd;

    shortEnd = p + ((end - p) < 8 ? (end - p) : 8);
    while (p < shortEnd && (lineOnly ? ak_str_sepline(*p) : ak_str_sep(*p)))
      p++;

    if (p < end && (lineOnly ? ak_str_sepline(*p) : ak_str_sep(*p)))
      p = ak_simd_skip_ascii_sep(p, end, lineOnly);

    while (p < end && (lineOnly ? ak_str_sepline(*p) : ak_str_sep(*p)))
      p++;
  } else {
    while (*p && (lineOnly ? ak_str_sepline(*p) : ak_str_sep(*p)))
      p++;
  }

  return p;
}

static inline
double
ak_str_pow10i(int exp) {
  static const double pow10[] = {
    1.0e1, 1.0e2, 1.0e4, 1.0e8, 1.0e16,
    1.0e32, 1.0e64, 1.0e128, 1.0e256
  };
  double value;
  unsigned int e, i;
  bool neg;

  if (exp == 0)
    return 1.0;

  neg = exp < 0;
  e   = neg ? (unsigned int)-exp : (unsigned int)exp;

  value = 1.0;
  for (i = 0; e && i < sizeof(pow10) / sizeof(pow10[0]); i++, e >>= 1) {
    if (e & 1u)
      value *= pow10[i];
  }

  return neg ? 1.0 / value : value;
}

static inline
char*
ak_str_parse_double_scalar(char     * __restrict p,
                           char     * __restrict end,
                           AkDouble * __restrict dest) {
  char   *begin;
  double  value, fracMul;
  int     exp;
  bool    neg, found;

  begin = p;
  neg   = false;
  if ((!end || p < end) && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value = 0.0;
  found = false;
  while ((!end || p < end) && ak_str_isdigit(*p)) {
    value = value * 10.0 + (double)(*p++ - '0');
    found = true;
  }

  if ((!end || p < end) && *p == '.') {
    p++;
    fracMul = 0.1;
    while ((!end || p < end) && ak_str_isdigit(*p)) {
      value  += (double)(*p++ - '0') * fracMul;
      fracMul *= 0.1;
      found = true;
    }
  }

  if (!found) {
    *dest = 0.0;
    return (!end && *begin == '\0') || (end && begin >= end)
           ? begin
           : begin + 1;
  }

  if ((!end || p < end) && (*p == 'e' || *p == 'E')) {
    char *expBegin;
    bool  expNeg, expFound;

    p++;
    expBegin = p;
    expNeg   = false;
    if ((!end || p < end) && (*p == '-' || *p == '+'))
      expNeg = *p++ == '-';

    exp = 0;
    expFound = false;
    while ((!end || p < end) && ak_str_isdigit(*p)) {
      if (exp < 10000)
        exp = exp * 10 + (*p - '0');
      p++;
      expFound = true;
    }

    if (expFound)
      value *= ak_str_pow10i(expNeg ? -exp : exp);
    else
      p = expBegin;
  }

  *dest = neg ? -value : value;

  return p;
}

static inline
char*
ak_str_parse_double(char     * __restrict p,
                    char     * __restrict end,
                    AkDouble * __restrict dest) {
  return ak_str_parse_double_scalar(p, end, dest);
}

static inline
char*
ak_str_parse_u64(char     * __restrict p,
                 char     * __restrict end,
                 AkUInt64 * __restrict dest) {
  char    *begin;
  AkUInt64 value;
  bool     neg, found;

  begin = p;
  neg   = false;
  if ((!end || p < end) && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value = 0;
  found = false;
  while ((!end || p < end) && ak_str_isdigit(*p)) {
    value = value * 10u + (AkUInt64)(*p++ - '0');
    found = true;
  }

  if (!found) {
    *dest = 0;
    return (!end && *begin == '\0') || (end && begin >= end)
           ? begin
           : begin + 1;
  }

  *dest = neg ? UINT64_MAX : value;

  return p;
}

static inline
char*
ak_str_parse_i64(char    * __restrict p,
                 char    * __restrict end,
                 AkInt64 * __restrict dest) {
  char    *begin;
  AkUInt64 value;
  bool     neg, found;

  begin = p;
  neg   = false;
  if ((!end || p < end) && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value = 0;
  found = false;
  while ((!end || p < end) && ak_str_isdigit(*p)) {
    value = value * 10u + (AkUInt64)(*p++ - '0');
    found = true;
  }

  if (!found) {
    *dest = 0;
    return (!end && *begin == '\0') || (end && begin >= end)
           ? begin
           : begin + 1;
  }

  *dest = neg ? -(AkInt64)value : (AkInt64)value;

  return p;
}

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
                     size_t            srclen,
                     size_t           *len) {
  int  i, count, itemc, buflen, found_sep;
  char c;

  if (srclen != 0)
    buflen = (int)srclen;
  else
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
unsigned long
ak_strtof(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkFloat * __restrict dest) {
  AkDouble value;
  AkFloat *out;
  char    *tok, *end;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      tok = ak_str_skip_sep(tok, end, false);
      if (tok >= end)
        break;
      tok = ak_str_parse_double(tok, end, &value);
      *out++ = (AkFloat)value;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep(tok, NULL, false);
      if (*tok == '\0')
        break;
      tok = ak_str_parse_double(tok, NULL, &value);
      *out++ = (AkFloat)value;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  return rem;
}

AK_EXPORT
unsigned long
ak_strtof_line(char    * __restrict src,
               size_t               srclen,
               unsigned long        n,
               AkFloat * __restrict dest) {
  AkDouble value;
  AkFloat *out;
  char    *tok, *end;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      tok = ak_str_skip_sep(tok, end, true);
      if (tok >= end)
        break;
      tok = ak_str_parse_double(tok, end, &value);
      *out++ = (AkFloat)value;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep(tok, NULL, true);
      if (*tok == '\0' || *tok == '\n' || *tok == '\r')
        break;
      tok = ak_str_parse_double(tok, NULL, &value);
      *out++ = (AkFloat)value;
      rem--;
    } while (rem > 0ul && *tok != '\0' && *tok != '\n' && *tok != '\r');
  }

  return rem;
}

AK_EXPORT
unsigned long
ak_strtod(char     * __restrict src,
          size_t                srclen,
          unsigned long         n,
          AkDouble * __restrict dest) {
  AkDouble *out;
  char     *tok, *end;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      tok = ak_str_skip_sep(tok, end, false);
      if (tok >= end)
        break;
      tok = ak_str_parse_double(tok, end, out++);
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep(tok, NULL, false);
      if (*tok == '\0')
        break;
      tok = ak_str_parse_double(tok, NULL, out++);
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  return rem;
}

AK_EXPORT
unsigned long
ak_strtoui(char    * __restrict src,
           size_t               srclen,
           unsigned long        n,
           AkUInt  * __restrict dest) {
  char    *tok, *end;
  AkUInt  *out;
  AkUInt64 val;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      tok = ak_str_skip_sep(tok, end, false);
      if (tok >= end)
        break;
      tok = ak_str_parse_u64(tok, end, &val);

      /* BUGFIX: some indices may come as -1 as BUG, fix this. */
      if (val < UINT32_MAX) {
        *out++ = (AkUInt)val;
      } else {
        *out++ = 0;
      }
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep(tok, NULL, false);
      if (*tok == '\0')
        break;
      tok = ak_str_parse_u64(tok, NULL, &val);

      /* BUGFIX: some indices may come as -1 as BUG, fix this. */
      if (val < UINT32_MAX) {
        *out++ = (AkUInt)val;
      } else {
        *out++ = 0;
      }
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  return rem;
}

AK_EXPORT
unsigned long
ak_strtoi(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkInt   * __restrict dest) {
  AkInt64 value;
  AkInt  *out;
  char   *tok, *end;
  unsigned long rem;
  
  if (n == 0)
    return 0;
  
  out = dest;
  tok = src;
  rem = n;
  
  if (srclen != 0) {
    end = src + srclen;
    
    do {
      tok = ak_str_skip_sep(tok, end, false);
      if (tok >= end)
        break;
      tok = ak_str_parse_i64(tok, end, &value);
      *out++ = (AkInt)value;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep(tok, NULL, false);
      if (*tok == '\0')
        break;
      tok = ak_str_parse_i64(tok, NULL, &value);
      *out++ = (AkInt)value;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }
  
  return rem;
}

AK_EXPORT
unsigned long
ak_strtoi_line(char    * __restrict src,
               size_t               srclen,
               unsigned long        n,
               AkInt   * __restrict dest) {
  AkInt64 value;
  AkInt  *out;
  char   *tok, *end;
  unsigned long rem;
  
  if (n == 0)
    return 0;
  
  out = dest;
  tok = src;
  rem = n;
  
  if (srclen != 0) {
    end = src + srclen;
    
    do {
      tok = ak_str_skip_sep(tok, end, true);
      if (tok >= end)
        break;
      tok = ak_str_parse_i64(tok, end, &value);
      *out++ = (AkInt)value;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep(tok, NULL, true);
      if (*tok == '\0' || *tok == '\n' || *tok == '\r')
        break;
      tok = ak_str_parse_i64(tok, NULL, &value);
      *out++ = (AkInt)value;
      rem--;
    } while (rem > 0ul && *tok != '\0' && *tok != '\n' && *tok != '\r');
  }
  
  return rem;
}

AK_EXPORT
unsigned long
ak_strtob(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkBool  * __restrict dest) {
  AkBool *out;
  char   *tok, *end;
  char    c;
  unsigned long rem;

  if (n == 0)
    return 0;
  
  out = dest;
  tok = src;
  rem = n;
  
  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      if (tok >= end)
        break;
      
      *out++ = tok[0] == 't' || tok[0] == 'T';
      tok++;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      if (*tok == '\0')
        break;
      
      *out++ = tok[0] == 't' || tok[0] == 'T';
      tok++;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }
  
  return rem;
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
