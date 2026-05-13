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

#ifndef assetkit_string_fast_h
#define assetkit_string_fast_h

#include "common.h"
#include "simd/scan.h"

AK_INLINE
bool
ak_str_isdigit_fast(char c) {
  return c >= '0' && c <= '9';
}

AK_INLINE
bool
ak_str_sep_fast(char c) {
  return c == ' ' || c == '\n' || c == '\t'
         || c == '\r' || c == '\f' || c == '\v';
}

AK_INLINE
bool
ak_str_sepline_fast(char c) {
  return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}

AK_INLINE
char*
ak_str_skip_sep_fast(char * __restrict p,
                     char * __restrict end,
                     bool              lineOnly) {
  if (end) {
    char *shortEnd;

    shortEnd = p + ((end - p) < 8 ? (end - p) : 8);
    while (p < shortEnd
           && (lineOnly ? ak_str_sepline_fast(*p) : ak_str_sep_fast(*p)))
      p++;

    if (p < end
        && (lineOnly ? ak_str_sepline_fast(*p) : ak_str_sep_fast(*p)))
      p = ak_simd_skip_ascii_sep(p, end, lineOnly);

    while (p < end
           && (lineOnly ? ak_str_sepline_fast(*p) : ak_str_sep_fast(*p)))
      p++;
  } else {
    while (*p && (lineOnly ? ak_str_sepline_fast(*p) : ak_str_sep_fast(*p)))
      p++;
  }

  return p;
}

AK_INLINE
double
ak_str_pow10i_fast(int exp) {
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

AK_INLINE
char*
ak_str_parse_double_fast(char     * __restrict p,
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
  while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
    value = value * 10.0 + (double)(*p++ - '0');
    found = true;
  }

  if ((!end || p < end) && *p == '.') {
    p++;
    fracMul = 0.1;
    while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
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
    while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
      if (exp < 10000)
        exp = exp * 10 + (*p - '0');
      p++;
      expFound = true;
    }

    if (expFound)
      value *= ak_str_pow10i_fast(expNeg ? -exp : exp);
    else
      p = expBegin;
  }

  *dest = neg ? -value : value;

  return p;
}

AK_INLINE
char*
ak_str_parse_u64_fast(char     * __restrict p,
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
  while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
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

AK_INLINE
char*
ak_str_parse_i64_fast(char    * __restrict p,
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
  while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
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

AK_INLINE
char*
ak_strtof_one_fast(char    * __restrict src,
                   AkFloat * __restrict dest) {
  AkDouble value;
  char    *tok;

  tok = ak_str_skip_sep_fast(src, NULL, false);
  if (*tok == '\0') {
    *dest = 0.0f;
    return tok;
  }

  tok = ak_str_parse_double_fast(tok, NULL, &value);
  *dest = (AkFloat)value;

  return tok;
}

AK_INLINE
char*
ak_strtoui_one_fast(char   * __restrict src,
                    AkUInt * __restrict dest) {
  AkUInt64 value;
  char    *tok;

  tok = ak_str_skip_sep_fast(src, NULL, false);
  if (*tok == '\0') {
    *dest = 0;
    return tok;
  }

  tok = ak_str_parse_u64_fast(tok, NULL, &value);
  *dest = value < UINT32_MAX ? (AkUInt)value : 0;

  return tok;
}

AK_INLINE
char*
ak_strtoi_one_fast(char  * __restrict src,
                   AkInt * __restrict dest) {
  AkInt64 value;
  char   *tok;

  tok = ak_str_skip_sep_fast(src, NULL, false);
  if (*tok == '\0') {
    *dest = 0;
    return tok;
  }

  tok = ak_str_parse_i64_fast(tok, NULL, &value);
  *dest = (AkInt)value;

  return tok;
}

#endif /* assetkit_string_fast_h */
