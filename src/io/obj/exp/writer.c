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

#include "writer.h"

#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

AK_HIDE
void
wobj_w_flush(WOBJExpWriter * __restrict w) {
  if (w->len == 0)
    return;

  if (w->result == AK_OK
      && fwrite(w->buffer, 1, w->len, w->file) != w->len)
    w->result = AK_ERR;

  w->len = 0;
}

AK_HIDE
void
wobj_w_raw(WOBJExpWriter * __restrict w,
           const void    * __restrict data,
           size_t                     len) {
  const unsigned char *src;

  src = data;
  while (len > 0) {
    size_t avail;
    size_t n;

    avail = sizeof(w->buffer) - w->len;
    if (avail == 0) {
      wobj_w_flush(w);
      avail = sizeof(w->buffer);
    }

    if (w->len == 0 && len >= sizeof(w->buffer)) {
      if (w->result == AK_OK && fwrite(src, 1, len, w->file) != len)
        w->result = AK_ERR;
      return;
    }

    n = len < avail ? len : avail;
    memcpy(w->buffer + w->len, src, n);
    w->len += n;
    src    += n;
    len    -= n;
  }
}

AK_HIDE
void
wobj_w_name(WOBJExpWriter * __restrict w,
            const char    * __restrict name) {
  const char *it;
  const char *span;

  if (!name || !*name) {
    wobj_w_lit(w, "unnamed");
    return;
  }

  it = name;
  while (*it) {
    span = it;
    while (*it && *it != '\n' && *it != '\r')
      it++;

    if (it > span)
      wobj_w_raw(w, span, (size_t)(it - span));

    if (*it == '\n' || *it == '\r') {
      wobj_w_ch(w, '_');
      it++;
    }
  }
}

AK_HIDE
void
wobj_w_uint(WOBJExpWriter * __restrict w, uint32_t val) {
  char     buf[16];
  uint32_t n;
  uint32_t i;

  i = sizeof(buf);
  do {
    buf[--i] = (char)('0' + (val % 10u));
    val /= 10u;
  } while (val);

  n = (uint32_t)(sizeof(buf) - i);
  wobj_w_raw(w, buf + i, n);
}

static
bool
wobj_w_normalize_number(char * __restrict buf, size_t * __restrict len) {
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

AK_HIDE
void
wobj_w_float(WOBJExpWriter * __restrict w, float val) {
  char   buf[48];
  int    len;
  size_t outLen;

  if (!isfinite(val)) {
    w->result = AK_ERR;
    return;
  }

  len = snprintf(buf, sizeof(buf), "%.6g", (double)val);
  if (len <= 0 || (size_t)len >= sizeof(buf)) {
    w->result = AK_ERR;
    return;
  }

  outLen = (size_t)len;
  if (!wobj_w_normalize_number(buf, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  wobj_w_raw(w, buf, outLen);
}
