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
#include "../../common/text_number.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static
bool
gltf_w_mem_append(GLTFExpWriter        * __restrict w,
                  const unsigned char  * __restrict data,
                  size_t                            len) {
  unsigned char *newMem;
  size_t         newCap;
  size_t         need;

  if (len == 0)
    return true;

  if ((size_t)-1 - w->memLen < len)
    return false;

  need = w->memLen + len;
  if (need > w->memCap) {
    newCap = w->memCap ? w->memCap : GLTF_EXP_WRITER_CAP;
    while (newCap < need) {
      if (newCap > ((size_t)-1 / 2u)) {
        newCap = need;
        break;
      }
      newCap *= 2u;
    }

    newMem = realloc(w->mem, newCap);
    if (!newMem)
      return false;

    w->mem    = newMem;
    w->memCap = newCap;
  }

  memcpy(w->mem + w->memLen, data, len);
  w->memLen += len;

  return true;
}

bool
gltf_w_init_memory(GLTFExpWriter * __restrict w,
                   size_t                     capacity) {
  memset(w, 0, sizeof(*w));
  w->result = AK_OK;
  w->memory = true;

  if (capacity == 0)
    return true;

  w->mem = malloc(capacity);
  if (!w->mem) {
    w->result = AK_ERR;
    return false;
  }

  w->memCap = capacity;

  return true;
}

void
gltf_w_free(GLTFExpWriter * __restrict w) {
  free(w->mem);
  w->mem    = NULL;
  w->memLen = 0;
  w->memCap = 0;
}

void
gltf_w_flush(GLTFExpWriter * __restrict w) {
  if (w->len == 0)
    return;

  if (w->result == AK_OK) {
    if (w->memory) {
      if (!gltf_w_mem_append(w, w->buffer, w->len))
        w->result = AK_ERR;
    } else if (fwrite(w->buffer, 1, w->len, w->file) != w->len) {
      w->result = AK_ERR;
    }
  }

  w->len = 0;
}

void
gltf_w_raw(GLTFExpWriter * __restrict w,
           const void    * __restrict data,
           size_t                     len) {
  const unsigned char *src;

  src = data;

  while (len > 0) {
    size_t avail;
    size_t n;

    avail = GLTF_EXP_WRITER_CAP - w->len;
    if (avail == 0) {
      gltf_w_flush(w);
      avail = GLTF_EXP_WRITER_CAP;
    }

    if (w->len == 0 && len >= GLTF_EXP_WRITER_CAP) {
      if (w->result == AK_OK) {
        if (w->memory) {
          if (!gltf_w_mem_append(w, src, len))
            w->result = AK_ERR;
        } else if (fwrite(src, 1, len, w->file) != len) {
          w->result = AK_ERR;
        }
      }
      return;
    }

    n = len < avail ? len : avail;
    memcpy(w->buffer + w->len, src, n);
    w->len += n;
    src    += n;
    len    -= n;
  }
}

void
gltf_w_qstr_len(GLTFExpWriter * __restrict w,
                const char    * __restrict str,
                size_t                     len) {
  size_t i;

  gltf_w_ch(w, '"');

  i = 0;
  while (i < len) {
    unsigned char c;
    size_t        spanStart;

    spanStart = i;
    while (i < len) {
      c = (unsigned char)str[i];
      if (c < 0x20 || c == '"' || c == '\\')
        break;
      i++;
    }

    if (i > spanStart)
      gltf_w_raw(w, str + spanStart, i - spanStart);

    if (i == len)
      break;

    c = (unsigned char)str[i++];
    switch (c) {
      case '"':
        gltf_w_raw(w, "\\\"", 2);
        break;
      case '\\':
        gltf_w_raw(w, "\\\\", 2);
        break;
      case '\b':
        gltf_w_raw(w, "\\b", 2);
        break;
      case '\f':
        gltf_w_raw(w, "\\f", 2);
        break;
      case '\n':
        gltf_w_raw(w, "\\n", 2);
        break;
      case '\r':
        gltf_w_raw(w, "\\r", 2);
        break;
      case '\t':
        gltf_w_raw(w, "\\t", 2);
        break;
      default:
        if (c < 0x20) {
          static const char hex[] = "0123456789abcdef";
          char esc[6];

          esc[0] = '\\';
          esc[1] = 'u';
          esc[2] = '0';
          esc[3] = '0';
          esc[4] = hex[c >> 4];
          esc[5] = hex[c & 0x0f];
          gltf_w_raw(w, esc, sizeof(esc));
        } else {
          gltf_w_ch(w, (char)c);
        }
        break;
    }
  }

  gltf_w_ch(w, '"');
}

void
gltf_w_qstr(GLTFExpWriter * __restrict w,
            const char    * __restrict str) {
  gltf_w_qstr_len(w, str ? str : "", str ? strlen(str) : 0);
}

void
gltf_w_key(GLTFExpWriter * __restrict w,
           const char    * __restrict key,
           size_t                     keyLen) {
  gltf_w_qstr_len(w, key, keyLen);
  gltf_w_ch(w, ':');
}

void
gltf_w_key_str(GLTFExpWriter * __restrict w,
               const char    * __restrict key,
               size_t                     keyLen,
               const char    * __restrict val) {
  gltf_w_key(w, key, keyLen);
  gltf_w_qstr(w, val);
}

void
gltf_w_uint(GLTFExpWriter * __restrict w, size_t val) {
  char   buf[32];
  size_t i;
  size_t start;

  i = sizeof(buf);
  do {
    buf[--i] = (char)('0' + (val % 10u));
    val /= 10u;
  } while (val > 0 && i > 0);

  start = i;
  gltf_w_raw(w, buf + start, sizeof(buf) - start);
}

void
gltf_w_key_uint(GLTFExpWriter * __restrict w,
                const char    * __restrict key,
                size_t                     keyLen,
                size_t                     val) {
  gltf_w_key(w, key, keyLen);
  gltf_w_uint(w, val);
}

void
gltf_w_key_bool(GLTFExpWriter * __restrict w,
                const char    * __restrict key,
                size_t                     keyLen,
                bool                       val) {
  gltf_w_key(w, key, keyLen);
  gltf_w_raw(w, val ? "true" : "false", val ? 4 : 5);
}

void
gltf_w_float(GLTFExpWriter * __restrict w, float val) {
  char   buf[48];
  int    len;
  size_t outLen;

  if (!isfinite(val)) {
    w->result = AK_ERR;
    return;
  }
  if (ak_io_text_format_fixed_float(buf, sizeof(buf), val, 9u, &outLen)) {
    gltf_w_raw(w, buf, outLen);
    return;
  }

  len = snprintf(buf, sizeof(buf), "%.9g", (double)val);
  if (len <= 0 || (size_t)len >= sizeof(buf)) {
    w->result = AK_ERR;
    return;
  }

  outLen = (size_t)len;
  if (!ak_io_text_normalize_number(buf, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  gltf_w_raw(w, buf, outLen);
}

void
gltf_w_number(GLTFExpWriter * __restrict w, double val) {
  char   buf[48];
  int    len;
  size_t outLen;

  if (!isfinite(val)) {
    w->result = AK_ERR;
    return;
  }

  len = snprintf(buf, sizeof(buf), "%.17g", val);
  if (len <= 0 || (size_t)len >= sizeof(buf)) {
    w->result = AK_ERR;
    return;
  }

  outLen = (size_t)len;
  if (!ak_io_text_normalize_number(buf, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  gltf_w_raw(w, buf, outLen);
}
