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

#define AK_STR_PACK4_CHARS(C0, C1, C2, C3)                                  \
  ((uint32_t)(uint8_t)(C0)                                                   \
   | (uint32_t)(uint8_t)(C1) << 8                                            \
   | (uint32_t)(uint8_t)(C2) << 16                                           \
   | (uint32_t)(uint8_t)(C3) << 24)

#define AK_STR_PACK8_CHARS(C0, C1, C2, C3, C4, C5, C6, C7)                  \
  ((uint64_t)AK_STR_PACK4_CHARS(C0, C1, C2, C3)                             \
   | (uint64_t)(uint8_t)(C4) << 32                                           \
   | (uint64_t)(uint8_t)(C5) << 40                                           \
   | (uint64_t)(uint8_t)(C6) << 48                                           \
   | (uint64_t)(uint8_t)(C7) << 56)

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define AK_STR_LOAD_LE 1
#elif defined(_WIN32) || defined(_M_IX86) || defined(_M_X64)                 \
      || defined(_M_ARM64) || defined(__i386__) || defined(__x86_64__)       \
      || defined(__aarch64__)
#  define AK_STR_LOAD_LE 1
#else
#  define AK_STR_LOAD_LE 0
#endif

AK_INLINE
uint32_t
ak_str_load4_fast(const char * __restrict s) {
  uint32_t out;

#if AK_STR_LOAD_LE
  memcpy(&out, s, sizeof(out));
  return out;
#else
  out = (uint32_t)(uint8_t)s[0]
      | (uint32_t)(uint8_t)s[1] << 8
      | (uint32_t)(uint8_t)s[2] << 16
      | (uint32_t)(uint8_t)s[3] << 24;
  return out;
#endif
}

AK_INLINE
uint64_t
ak_str_load8_fast(const char * __restrict s) {
  uint64_t out;

#if AK_STR_LOAD_LE
  memcpy(&out, s, sizeof(out));
  return out;
#else
  out = (uint64_t)ak_str_load4_fast(s)
      | (uint64_t)ak_str_load4_fast(s + 4) << 32;
  return out;
#endif
}

AK_INLINE
bool
ak_str_isdigit_fast(char c) {
  return c >= '0' && c <= '9';
}

AK_INLINE
char
ak_str_ascii_lower_fast(char c) {
  return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

AK_INLINE
uint32_t
ak_str_pack4_fast(const char * __restrict s,
                  size_t                  len) {
  switch (len) {
    case 0:
      return 0;
    case 1:
      return (uint32_t)(uint8_t)s[0];
    case 2:
      return (uint32_t)(uint8_t)s[0]
           | (uint32_t)(uint8_t)s[1] << 8;
    case 3:
      return (uint32_t)(uint8_t)s[0]
           | (uint32_t)(uint8_t)s[1] << 8
           | (uint32_t)(uint8_t)s[2] << 16;
    default:
      return ak_str_load4_fast(s);
  }
}

AK_INLINE
uint32_t
ak_str_pack4z_fast(const char * __restrict s) {
  uint32_t out;

  if (!s[0])
    return 0;

  out = (uint32_t)(uint8_t)s[0];
  if (!s[1])
    return out;

  out |= (uint32_t)(uint8_t)s[1] << 8;
  if (!s[2])
    return out;

  out |= (uint32_t)(uint8_t)s[2] << 16;
  if (!s[3])
    return out;

  out |= (uint32_t)(uint8_t)s[3] << 24;

  return out;
}

AK_INLINE
uint64_t
ak_str_pack8_fast(const char * __restrict s,
                  size_t                  len) {
  switch (len) {
    case 0:
      return 0;
    case 1:
      return (uint64_t)(uint8_t)s[0];
    case 2:
      return (uint64_t)ak_str_pack4_fast(s, 2);
    case 3:
      return (uint64_t)ak_str_pack4_fast(s, 3);
    case 4:
      return (uint64_t)ak_str_pack4_fast(s, 4);
    case 5:
      return (uint64_t)ak_str_load4_fast(s)
           | (uint64_t)(uint8_t)s[4] << 32;
    case 6:
      return (uint64_t)ak_str_load4_fast(s)
           | (uint64_t)(uint8_t)s[4] << 32
           | (uint64_t)(uint8_t)s[5] << 40;
    case 7:
      return (uint64_t)ak_str_load4_fast(s)
           | (uint64_t)(uint8_t)s[4] << 32
           | (uint64_t)(uint8_t)s[5] << 40
           | (uint64_t)(uint8_t)s[6] << 48;
    default:
      return ak_str_load8_fast(s);
  }
}

AK_INLINE
uint32_t
ak_str_pack4_ci_fast(const char * __restrict s,
                     size_t                  len) {
  switch (len) {
    case 0:
      return 0;
    case 1:
      return (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[0]);
    case 2:
      return (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[0])
           | (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[1]) << 8;
    case 3:
      return (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[0])
           | (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[1]) << 8
           | (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[2]) << 16;
    default:
      return (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[0])
           | (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[1]) << 8
           | (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[2]) << 16
           | (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[3]) << 24;
  }
}

AK_INLINE
uint32_t
ak_str_pack4z_ci_fast(const char * __restrict s) {
  uint32_t out;

  if (!s[0])
    return 0;

  out = (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[0]);
  if (!s[1])
    return out;

  out |= (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[1]) << 8;
  if (!s[2])
    return out;

  out |= (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[2]) << 16;
  if (!s[3])
    return out;

  out |= (uint32_t)(uint8_t)ak_str_ascii_lower_fast(s[3]) << 24;

  return out;
}

AK_INLINE
uint64_t
ak_str_pack8_ci_fast(const char * __restrict s,
                     size_t                  len) {
  switch (len) {
    case 0:
      return 0;
    case 1:
      return (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[0]);
    case 2:
      return (uint64_t)ak_str_pack4_ci_fast(s, 2);
    case 3:
      return (uint64_t)ak_str_pack4_ci_fast(s, 3);
    case 4:
      return (uint64_t)ak_str_pack4_ci_fast(s, 4);
    case 5:
      return (uint64_t)ak_str_pack4_ci_fast(s, 4)
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[4]) << 32;
    case 6:
      return (uint64_t)ak_str_pack4_ci_fast(s, 4)
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[4]) << 32
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[5]) << 40;
    case 7:
      return (uint64_t)ak_str_pack4_ci_fast(s, 4)
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[4]) << 32
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[5]) << 40
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[6]) << 48;
    default:
      return (uint64_t)ak_str_pack4_ci_fast(s, 4)
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[4]) << 32
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[5]) << 40
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[6]) << 48
           | (uint64_t)(uint8_t)ak_str_ascii_lower_fast(s[7]) << 56;
  }
}

AK_INLINE
bool
ak_str_eq_packed_fast(const char * __restrict s,
                      size_t                  len,
                      uint64_t                packed,
                      size_t                  litLen) {
  if (!s || len != litLen || litLen > 8)
    return false;

  if (litLen > 0 && (uint8_t)s[0] != (uint8_t)packed)
    return false;

  return len <= 4
         ? (uint64_t)ak_str_pack4_fast(s, len) == packed
         : ak_str_pack8_fast(s, len) == packed;
}

AK_INLINE
bool
ak_str_eq_packed_ci_fast(const char * __restrict s,
                         size_t                  len,
                         uint64_t                packed,
                         size_t                  litLen) {
  if (!s || len != litLen || litLen > 8)
    return false;

  if (litLen > 0
      && (uint8_t)ak_str_ascii_lower_fast(s[0]) != (uint8_t)packed)
    return false;

  return len <= 4
         ? (uint64_t)ak_str_pack4_ci_fast(s, len) == packed
         : ak_str_pack8_ci_fast(s, len) == packed;
}

AK_INLINE
bool
ak_str_eq_fast(const char * __restrict s,
               size_t                  len,
               const char * __restrict lit,
               size_t                  litLen) {
  if (!s || !lit || len != litLen)
    return false;

  return len == 0 || (s[0] == lit[0] && memcmp(s, lit, len) == 0);
}

AK_INLINE
bool
ak_str_eq_cstr_fast(const char * __restrict s,
                    const char * __restrict lit,
                    size_t                  litLen) {
  size_t i;

  if (!s || !lit)
    return false;

  for (i = 0; i < litLen; i++) {
    if (s[i] == '\0' || s[i] != lit[i])
      return false;
  }

  return s[litLen] == '\0';
}

AK_INLINE
const char*
ak_str_find_char_fast(const char * __restrict s, char ch) {
  if (!s)
    return NULL;

  while (*s) {
    if (*s == ch)
      return s;
    s++;
  }

  return ch == '\0' ? s : NULL;
}

AK_INLINE
bool
ak_str_has_char_fast(const char * __restrict s, char ch) {
  return ak_str_find_char_fast(s, ch) != NULL;
}

AK_INLINE
bool
ak_str_eq_ci_fast(const char * __restrict s,
                  const char * __restrict lit,
                  size_t                  len) {
  size_t i;

  if (memcmp(s, lit, len) == 0)
    return true;

  for (i = 0; i < len; i++) {
    if (ak_str_ascii_lower_fast(s[i]) != ak_str_ascii_lower_fast(lit[i]))
      return false;
  }

  return true;
}

AK_INLINE
bool
ak_str_eq_ci_len_fast(const char * __restrict s,
                      size_t                  len,
                      const char * __restrict lit,
                      size_t                  litLen) {
  return len == litLen && ak_str_eq_ci_fast(s, lit, len);
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
float
ak_str_pow10if_fast(int exp) {
  static const float pow10[] = {
    1.0e1f, 1.0e2f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f
  };
  float value;
  unsigned int e, i;
  bool neg;

  if (exp == 0)
    return 1.0f;

  neg = exp < 0;
  e   = neg ? (unsigned int)-exp : (unsigned int)exp;

  value = 1.0f;
  for (i = 0; e && i < AK_ARRAY_LEN(pow10); i++, e >>= 1) {
    if (e & 1u)
      value *= pow10[i];
  }

  if (e)
    return neg ? 0.0f : FLT_MAX;

  return neg ? 1.0f / value : value;
}

AK_INLINE
bool
ak_str_parse_u32_slice_fast(const char * __restrict p,
                            const char * __restrict end,
                            uint32_t   * __restrict dest) {
  uint64_t value;
  bool     found;

  if (!p || !end || !dest || p >= end)
    return false;

  value = 0u;
  found = false;
  while (p < end && ak_str_isdigit_fast(*p)) {
    value = value * 10u + (uint64_t)(*p++ - '0');
    if (value > UINT32_MAX)
      return false;
    found = true;
  }

  if (!found || p != end)
    return false;

  *dest = (uint32_t)value;
  return true;
}

AK_INLINE
bool
ak_str_parse_u32_quoted_fast(const char  * __restrict p,
                             const char  * __restrict end,
                             char                       quote,
                             uint32_t    * __restrict dest,
                             const char ** __restrict afterQuote) {
  uint64_t value;
  bool     found;

  if (!p || !end || !dest || !afterQuote || p >= end)
    return false;

  value = 0u;
  found = false;
  while (p < end && ak_str_isdigit_fast(*p)) {
    value = value * 10u + (uint64_t)(*p++ - '0');
    if (value > UINT32_MAX)
      return false;
    found = true;
  }

  if (!found || p >= end || *p != quote)
    return false;

  *dest       = (uint32_t)value;
  *afterQuote = p + 1u;
  return true;
}

AK_INLINE
bool
ak_str_parse_simple_float_quoted_fast(const char  * __restrict p,
                                      const char  * __restrict end,
                                      char                       quote,
                                      AkFloat     * __restrict dest,
                                      const char ** __restrict afterQuote) {
  static const float invPow10[] = {
    1.0f,
    1.0e-1f,
    1.0e-2f,
    1.0e-3f,
    1.0e-4f,
    1.0e-5f,
    1.0e-6f,
    1.0e-7f,
    1.0e-8f,
    1.0e-9f,
    1.0e-10f,
    1.0e-11f,
    1.0e-12f,
    1.0e-13f,
    1.0e-14f,
    1.0e-15f,
    1.0e-16f,
    1.0e-17f,
    1.0e-18f
  };
  uint64_t intValue;
  uint64_t fracValue;
  uint32_t fracDigits;
  bool     neg;
  bool     found;
  float    value;

  if (!p || !end || !dest || !afterQuote || p >= end)
    return false;

  neg = false;
  if (*p == '-' || *p == '+')
    neg = *p++ == '-';

  intValue = 0u;
  found    = false;
  while (p < end && ak_str_isdigit_fast(*p)) {
    if (intValue > (UINT64_MAX - (uint64_t)(*p - '0')) / 10u)
      return false;
    intValue = intValue * 10u + (uint64_t)(*p++ - '0');
    found = true;
  }

  fracValue  = 0u;
  fracDigits = 0u;
  if (p < end && *p == '.') {
    p++;
    while (p < end && ak_str_isdigit_fast(*p)) {
      if (fracDigits + 1u >= AK_ARRAY_LEN(invPow10))
        return false;
      fracValue = fracValue * 10u + (uint64_t)(*p++ - '0');
      fracDigits++;
      found = true;
    }
  }

  if (!found || p >= end || *p != quote)
    return false;

  value = (float)intValue;
  if (fracDigits > 0u)
    value += (float)fracValue * invPow10[fracDigits];
  *dest       = neg ? -value : value;
  *afterQuote = p + 1u;
  return true;
}

AK_INLINE
char*
ak_str_parse_float_fast(char    * __restrict p,
                        char    * __restrict end,
                        AkFloat * __restrict dest) {
  char  *begin;
  float  value, fracMul;
  int    exp;
  bool   neg, found;

  begin = p;
  neg   = false;
  if ((!end || p < end) && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value = 0.0f;
  found = false;
  while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
    value = value * 10.0f + (float)(*p++ - '0');
    found = true;
  }

  if ((!end || p < end) && *p == '.') {
    p++;
    fracMul = 0.1f;
    while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
      value  += (float)(*p++ - '0') * fracMul;
      fracMul *= 0.1f;
      found = true;
    }
  }

  if (!found) {
    *dest = 0.0f;
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
      value *= ak_str_pow10if_fast(expNeg ? -exp : exp);
    else
      p = expBegin;
  }

  *dest = neg ? -value : value;

  return p;
}

AK_INLINE
char*
ak_str_parse_float_end_fast(char    * __restrict p,
                            char    * __restrict end,
                            AkFloat * __restrict dest) {
  char  *begin;
  float  value, fracMul;
  int    exp;
  bool   neg, found;

  begin = p;
  neg   = false;
  if (p < end && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value = 0.0f;
  found = false;
  while (p < end && ak_str_isdigit_fast(*p)) {
    value = value * 10.0f + (float)(*p++ - '0');
    found = true;
  }

  if (p < end && *p == '.') {
    p++;
    fracMul = 0.1f;
    while (p < end && ak_str_isdigit_fast(*p)) {
      value += (float)(*p++ - '0') * fracMul;
      fracMul *= 0.1f;
      found = true;
    }
  }

  if (!found) {
    *dest = 0.0f;
    return begin >= end ? begin : begin + 1;
  }

  if (p < end && (*p == 'e' || *p == 'E')) {
    char *expBegin;
    bool  expNeg, expFound;

    p++;
    expBegin = p;
    expNeg   = false;
    if (p < end && (*p == '-' || *p == '+'))
      expNeg = *p++ == '-';

    exp = 0;
    expFound = false;
    while (p < end && ak_str_isdigit_fast(*p)) {
      if (exp < 10000)
        exp = exp * 10 + (*p - '0');
      p++;
      expFound = true;
    }

    if (expFound)
      value *= ak_str_pow10if_fast(expNeg ? -exp : exp);
    else
      p = expBegin;
  }

  *dest = neg ? -value : value;

  return p;
}

AK_INLINE
bool
ak_str_float_token_start_fast(const char * __restrict p,
                              const char * __restrict end) {
  if (!p || !end || p >= end)
    return false;

  if (*p == '-' || *p == '+')
    p++;

  if (p >= end)
    return false;

  return ak_str_isdigit_fast(*p)
         || (*p == '.' && p + 1u < end && ak_str_isdigit_fast(p[1]));
}

AK_INLINE
bool
ak_str_parse_float_token_fast(const char  * __restrict p,
                              const char  * __restrict end,
                              AkFloat     * __restrict dest,
                              const char ** __restrict afterToken) {
  char *parsed;

  if (!dest || !afterToken || !ak_str_float_token_start_fast(p, end))
    return false;

  parsed = ak_str_parse_float_end_fast((char *)p, (char *)end, dest);
  if ((const char *)parsed <= p)
    return false;

  *afterToken = parsed;
  return true;
}

AK_INLINE
bool
ak_str_parse_float_quoted_fast(const char  * __restrict p,
                               const char  * __restrict end,
                               char                       quote,
                               AkFloat     * __restrict dest,
                               const char ** __restrict afterQuote) {
  char *parsed;

  if (ak_str_parse_simple_float_quoted_fast(p, end, quote, dest, afterQuote))
    return true;

  if (!p || !end || !dest || !afterQuote || p >= end
      || !ak_str_float_token_start_fast(p, end))
    return false;

  parsed = ak_str_parse_float_end_fast((char *)p, (char *)end, dest);
  if ((const char *)parsed <= p || (const char *)parsed >= end
      || *parsed != quote)
    return false;

  *afterQuote = parsed + 1u;
  return true;
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
ak_str_parse_uint_fast(char   * __restrict p,
                       char   * __restrict end,
                       AkUInt * __restrict dest) {
  char   *begin;
  AkUInt  value, digit;
  bool    neg, found, overflow;

  begin = p;
  neg   = false;
  if ((!end || p < end) && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value    = 0;
  found    = false;
  overflow = false;
  while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
    digit = (AkUInt)(*p++ - '0');
    if (!overflow) {
      if (value > UINT32_MAX / 10u
          || (value == UINT32_MAX / 10u && digit > UINT32_MAX % 10u)) {
        overflow = true;
      } else {
        value = value * 10u + digit;
      }
    }
    found = true;
  }

  if (!found) {
    *dest = 0;
    return (!end && *begin == '\0') || (end && begin >= end)
           ? begin
           : begin + 1;
  }

  *dest = (!neg && !overflow) ? value : 0;

  return p;
}

AK_INLINE
char*
ak_str_parse_uint_end_fast(char   * __restrict p,
                           char   * __restrict end,
                           AkUInt * __restrict dest) {
  char   *begin;
  AkUInt  value, digit;
  bool    neg, found, overflow;

  begin = p;
  neg   = false;
  if (p < end && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value    = 0;
  found    = false;
  overflow = false;
  while (p < end && ak_str_isdigit_fast(*p)) {
    digit = (AkUInt)(*p++ - '0');
    if (!overflow) {
      if (value > UINT32_MAX / 10u
          || (value == UINT32_MAX / 10u && digit > UINT32_MAX % 10u)) {
        overflow = true;
      } else {
        value = value * 10u + digit;
      }
    }
    found = true;
  }

  if (!found) {
    *dest = 0;
    return begin >= end ? begin : begin + 1;
  }

  *dest = (!neg && !overflow) ? value : 0;

  return p;
}

AK_INLINE
char*
ak_str_parse_uint_index_fast(char   * __restrict p,
                             char   * __restrict end,
                             AkUInt * __restrict dest) {
  char  *begin;
  AkUInt value;
  bool   found;

  begin = p;
  value = 0;
  found = false;

  while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
    value = value * 10u + (AkUInt)(*p++ - '0');
    found = true;
  }

  if (!found) {
    *dest = 0;
    return (!end && *begin == '\0') || (end && begin >= end)
           ? begin
           : begin + 1;
  }

  *dest = value;

  return p;
}

AK_INLINE
bool
ak_str_parse_positive_i32_token_fast(char  ** __restrict pp,
                                     AkInt  * __restrict out) {
  char   *p;
  AkUInt  value;

  p = *pp;
  if (!ak_str_isdigit_fast(*p))
    return false;

  value = 0;
  do {
    value = value * 10u + (AkUInt)(*p++ - '0');
    if (value > (AkUInt)INT32_MAX)
      return false;
  } while (ak_str_isdigit_fast(*p));

  if (value == 0)
    return false;

  *out = (AkInt)value;
  *pp  = p;
  return true;
}

AK_INLINE
char*
ak_str_parse_uint_index_end_fast(char   * __restrict p,
                                 char   * __restrict end,
                                 AkUInt * __restrict dest) {
  char  *begin;
  AkUInt value;
  bool   found;

  begin = p;
  value = 0;
  found = false;

  while (p < end && ak_str_isdigit_fast(*p)) {
    value = value * 10u + (AkUInt)(*p++ - '0');
    found = true;
  }

  if (!found) {
    *dest = 0;
    return begin >= end ? begin : begin + 1;
  }

  *dest = value;

  return p;
}

AK_INLINE
char*
ak_str_parse_int_fast(char  * __restrict p,
                      char  * __restrict end,
                      AkInt * __restrict dest) {
  char  *begin;
  AkInt  value, digit;
  bool   neg, found, overflow;

  begin = p;
  neg   = false;
  if ((!end || p < end) && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value    = 0;
  found    = false;
  overflow = false;
  while ((!end || p < end) && ak_str_isdigit_fast(*p)) {
    digit = (AkInt)(*p++ - '0');
    if (!overflow) {
      if (neg) {
        if (value < INT32_MIN / 10
            || (value == INT32_MIN / 10 && digit > 8)) {
          overflow = true;
        } else {
          value = value * 10 - digit;
        }
      } else {
        if (value > INT32_MAX / 10
            || (value == INT32_MAX / 10 && digit > 7)) {
          overflow = true;
        } else {
          value = value * 10 + digit;
        }
      }
    }
    found = true;
  }

  if (!found) {
    *dest = 0;
    return (!end && *begin == '\0') || (end && begin >= end)
           ? begin
           : begin + 1;
  }

  *dest = overflow ? (neg ? INT32_MIN : INT32_MAX) : value;

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
unsigned long
ak_str_parse_float_array_fast(char     * __restrict src,
                              size_t                 srclen,
                              unsigned long          n,
                              AkFloat  * __restrict dest,
                              bool                   lineOnly) {
  AkFloat      *out;
  char         *tok;
  char         *end;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;
    do {
      tok = ak_str_skip_sep_fast(tok, end, lineOnly);
      if (tok >= end || (lineOnly && (*tok == '\n' || *tok == '\r')))
        break;
      tok = ak_str_parse_float_end_fast(tok, end, out++);
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, lineOnly);
      if (*tok == '\0' || (lineOnly && (*tok == '\n' || *tok == '\r')))
        break;
      tok = ak_str_parse_float_fast(tok, NULL, out++);
      rem--;
    } while (rem > 0ul
             && *tok != '\0'
             && (!lineOnly || (*tok != '\n' && *tok != '\r')));
  }

  return rem;
}

AK_INLINE
unsigned long
ak_str_parse_double_array_fast(char      * __restrict src,
                               size_t                  srclen,
                               unsigned long           n,
                               AkDouble  * __restrict dest) {
  AkDouble     *out;
  char         *tok;
  char         *end;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;
    do {
      tok = ak_str_skip_sep_fast(tok, end, false);
      if (tok >= end)
        break;
      tok = ak_str_parse_double_fast(tok, end, out++);
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, false);
      if (*tok == '\0')
        break;
      tok = ak_str_parse_double_fast(tok, NULL, out++);
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  return rem;
}

AK_INLINE
unsigned long
ak_str_parse_uint_array_fast(char     * __restrict src,
                             size_t                 srclen,
                             unsigned long          n,
                             AkUInt   * __restrict dest,
                             AkUInt   * __restrict maxValue) {
  AkUInt       *out;
  char         *tok;
  char         *end;
  AkUInt        value;
  AkUInt        maxv;
  unsigned long rem;

  if (n == 0)
    return 0;

  out  = dest;
  tok  = src;
  rem  = n;
  maxv = maxValue ? *maxValue : 0;

  if (srclen != 0) {
    end = src + srclen;
    do {
      tok = ak_str_skip_sep_fast(tok, end, false);
      if (tok >= end)
        break;
      tok = ak_str_parse_uint_end_fast(tok, end, &value);
      *out++ = value;
      if (value > maxv)
        maxv = value;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, false);
      if (*tok == '\0')
        break;
      tok = ak_str_parse_uint_fast(tok, NULL, &value);
      *out++ = value;
      if (value > maxv)
        maxv = value;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  if (maxValue)
    *maxValue = maxv;

  return rem;
}

AK_INLINE
unsigned long
ak_str_parse_int_array_fast(char    * __restrict src,
                            size_t                srclen,
                            unsigned long         n,
                            AkInt   * __restrict dest,
                            bool                  lineOnly) {
  AkInt        *out;
  char         *tok;
  char         *end;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;
    do {
      tok = ak_str_skip_sep_fast(tok, end, lineOnly);
      if (tok >= end || (lineOnly && (*tok == '\n' || *tok == '\r')))
        break;
      tok = ak_str_parse_int_fast(tok, end, out++);
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, lineOnly);
      if (*tok == '\0' || (lineOnly && (*tok == '\n' || *tok == '\r')))
        break;
      tok = ak_str_parse_int_fast(tok, NULL, out++);
      rem--;
    } while (rem > 0ul
             && *tok != '\0'
             && (!lineOnly || (*tok != '\n' && *tok != '\r')));
  }

  return rem;
}

AK_INLINE
char*
ak_strtof_one_fast(char    * __restrict src,
                   AkFloat * __restrict dest) {
  char *tok;

  tok = ak_str_skip_sep_fast(src, NULL, false);
  if (*tok == '\0') {
    *dest = 0.0f;
    return tok;
  }

  return ak_str_parse_float_fast(tok, NULL, dest);
}

AK_INLINE
char*
ak_strtoui_one_fast(char   * __restrict src,
                    AkUInt * __restrict dest) {
  char *tok;

  tok = ak_str_skip_sep_fast(src, NULL, false);
  if (*tok == '\0') {
    *dest = 0;
    return tok;
  }

  return ak_str_parse_uint_fast(tok, NULL, dest);
}

AK_INLINE
char*
ak_strtoi_one_fast(char  * __restrict src,
                   AkInt * __restrict dest) {
  char *tok;

  tok = ak_str_skip_sep_fast(src, NULL, false);
  if (*tok == '\0') {
    *dest = 0;
    return tok;
  }

  return ak_str_parse_int_fast(tok, NULL, dest);
}

#endif /* assetkit_string_fast_h */
