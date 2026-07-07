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

#ifndef assetkit_io_common_text_number_h
#define assetkit_io_common_text_number_h

#include "../../common.h"

#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static inline
char*
ak_io_text_format_uint64(char * __restrict p, uint64_t val) {
  char tmp[24];
  size_t i;

  i = sizeof(tmp);
  do {
    tmp[--i] = (char)('0' + (val % 10u));
    val /= 10u;
  } while (val);

  while (i < sizeof(tmp))
    *p++ = tmp[i++];

  return p;
}

static inline
char*
ak_io_text_format_uint32(char * __restrict p, uint32_t val) {
  static const char digits[201] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";
  char tmp[16];
  char *out;
  size_t len;

  out = tmp + sizeof(tmp);
  while (val >= 100u) {
    uint32_t next;
    uint32_t rem;

    next = val / 100u;
    rem  = val - next * 100u;
    out -= 2u;
    memcpy(out, digits + rem * 2u, 2u);
    val = next;
  }

  if (val < 10u) {
    *--out = (char)('0' + val);
  } else {
    out -= 2u;
    memcpy(out, digits + val * 2u, 2u);
  }

  len = (size_t)((tmp + sizeof(tmp)) - out);
  memcpy(p, out, len);
  p += len;

  return p;
}

static inline
bool
ak_io_text_normalize_number(char * __restrict buf, size_t * __restrict len) {
  size_t i;

  for (i = 0; i < *len; i++) {
    char c;

    c = buf[i];
    if ((c >= '0' && c <= '9')
        || c == '.'
        || c == '-'
        || c == '+'
        || c == 'e'
        || c == 'E') {
      continue;
    }

    if (c == ',') {
      buf[i] = '.';
      continue;
    }

    {
      struct lconv *lc;
      const char   *decimalPoint;
      size_t        decimalLen;

      lc           = localeconv();
      decimalPoint = lc ? lc->decimal_point : NULL;
      decimalLen   = decimalPoint ? strlen(decimalPoint) : 0;
      if (decimalLen == 0
          || decimalPoint[0] == '.'
          || decimalLen > *len - i
          || memcmp(buf + i, decimalPoint, decimalLen) != 0)
        return false;

      buf[i] = '.';
      if (decimalLen > 1u) {
        memmove(buf + i + 1u,
                buf + i + decimalLen,
                *len - i - decimalLen);
        *len -= decimalLen - 1u;
        buf[*len] = '\0';
      }
    }
  }

  return true;
}

static inline
bool
ak_io_text_format_fixed_float(char     * __restrict buf,
                              size_t                cap,
                              float                 val,
                              uint32_t              precision,
                              size_t   * __restrict outLen) {
  static const double scales[] = {
    1.0,
    10.0,
    100.0,
    1000.0,
    10000.0,
    100000.0,
    1000000.0,
    10000000.0,
    100000000.0,
    1000000000.0
  };
  char     frac[9];
  char    *p;
  double   scale;
  double   absVal;
  double   scaledDouble;
  uint64_t scaled;
  uint64_t intPart;
  uint64_t scaleInt;
  uint32_t fracPart;
  int      fracLen;

  if (!buf || cap < 32u || !outLen || precision >= AK_ARRAY_LEN(scales))
    return false;
  if (val == 0.0f) {
    buf[0] = '0';
    *outLen = 1u;
    return true;
  }

  scale  = scales[precision];
  absVal = val < 0.0f ? -(double)val : (double)val;
  if (absVal < 0.5 / scale)
    return false;
  if (absVal >= 9007199254740991.0 / scale)
    return false;

  scaledDouble = absVal * scale + 0.5;
  if (scaledDouble > 9007199254740991.0)
    return false;

  scaleInt = (uint64_t)scale;
  scaled   = (uint64_t)scaledDouble;
  intPart  = scaled / scaleInt;
  fracPart = (uint32_t)(scaled - intPart * scaleInt);

  p = buf;
  if (val < 0.0f)
    *p++ = '-';
  p = ak_io_text_format_uint64(p, intPart);

  if (fracPart) {
    for (int i = (int)precision - 1; i >= 0; i--) {
      frac[i] = (char)('0' + (fracPart % 10u));
      fracPart /= 10u;
    }
    fracLen = (int)precision;
    while (fracLen > 0 && frac[fracLen - 1] == '0')
      fracLen--;
    *p++ = '.';
    memcpy(p, frac, (size_t)fracLen);
    p += fracLen;
  }

  *outLen = (size_t)(p - buf);
  return true;
}

static inline
bool
ak_io_text_format_float6(char     * __restrict buf,
                         size_t                cap,
                         float                 val,
                         size_t   * __restrict outLen) {
  int len;

  if (!isfinite(val))
    return false;
  if (ak_io_text_format_fixed_float(buf, cap, val, 6u, outLen))
    return true;

  len = snprintf(buf, cap, "%.6g", (double)val);
  if (len <= 0 || (size_t)len >= cap)
    return false;

  *outLen = (size_t)len;
  return ak_io_text_normalize_number(buf, outLen);
}

static inline
bool
ak_io_text_format_float9(char     * __restrict buf,
                         size_t                cap,
                         float                 val,
                         size_t   * __restrict outLen) {
  int len;

  if (!isfinite(val))
    return false;
  if (ak_io_text_format_fixed_float(buf, cap, val, 9u, outLen))
    return true;

  len = snprintf(buf, cap, "%.9g", (double)val);
  if (len <= 0 || (size_t)len >= cap)
    return false;

  *outLen = (size_t)len;
  return ak_io_text_normalize_number(buf, outLen);
}

static inline
bool
ak_io_text_format_double15(char     * __restrict buf,
                           size_t                cap,
                           double                val,
                           size_t   * __restrict outLen) {
  int len;

  if (!isfinite(val))
    return false;

  len = snprintf(buf, cap, "%.15g", val);
  if (len <= 0 || (size_t)len >= cap)
    return false;

  *outLen = (size_t)len;
  return ak_io_text_normalize_number(buf, outLen);
}

static inline
bool
ak_io_text_format_double17(char     * __restrict buf,
                           size_t                cap,
                           double                val,
                           size_t   * __restrict outLen) {
  int len;

  if (!isfinite(val))
    return false;

  len = snprintf(buf, cap, "%.17g", val);
  if (len <= 0 || (size_t)len >= cap)
    return false;

  *outLen = (size_t)len;
  return ak_io_text_normalize_number(buf, outLen);
}

#endif /* assetkit_io_common_text_number_h */
