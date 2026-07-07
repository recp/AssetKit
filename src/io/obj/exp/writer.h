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

#ifndef assetkit_obj_exp_writer_h
#define assetkit_obj_exp_writer_h

#include "common.h"
#include "../../common/text_number.h"

AK_HIDE
void
wobj_w_flush(WOBJExpWriter * __restrict w);

AK_HIDE
void
wobj_w_raw(WOBJExpWriter * __restrict w,
           const void    * __restrict data,
           size_t                     len);

#define WOBJ_W_LIT(W, LIT) wobj_w_raw((W), "" LIT, sizeof("" LIT) - 1u)

AK_INLINE
void
wobj_w_ch(WOBJExpWriter * __restrict w, char ch) {
  if (w->len == sizeof(w->buffer))
    wobj_w_flush(w);

  w->buffer[w->len++] = (unsigned char)ch;
}

AK_INLINE
void
wobj_w_ch2(WOBJExpWriter * __restrict w, char a, char b) {
  if (sizeof(w->buffer) - w->len < 2u)
    wobj_w_flush(w);

  w->buffer[w->len++] = (unsigned char)a;
  w->buffer[w->len++] = (unsigned char)b;
}

AK_INLINE
void
wobj_w_lit(WOBJExpWriter * __restrict w,
           const char    * __restrict lit) {
  wobj_w_raw(w, lit, strlen(lit));
}

AK_INLINE
void
wobj_w_uint_fast(WOBJExpWriter * __restrict w, uint32_t val) {
  char     buf[16];
  uint32_t n;
  uint32_t i;

  i = sizeof(buf);
  do {
    buf[--i] = (char)('0' + (val % 10u));
    val /= 10u;
  } while (val);

  n = (uint32_t)(sizeof(buf) - i);
  if (sizeof(w->buffer) - w->len < n)
    wobj_w_flush(w);

  memcpy(w->buffer + w->len, buf + i, n);
  w->len += n;
}

AK_INLINE
void
wobj_w_float_fast(WOBJExpWriter * __restrict w, float val) {
  char  *dst;
  size_t avail;
  size_t outLen;

  avail = sizeof(w->buffer) - w->len;
  if (avail < 48u) {
    wobj_w_flush(w);
    avail = sizeof(w->buffer) - w->len;
  }

  dst = (char *)w->buffer + w->len;
  if (!ak_io_text_format_float6(dst, avail, val, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  w->len += outLen;
}

AK_HIDE
void
wobj_w_name(WOBJExpWriter * __restrict w,
            const char    * __restrict name);

#endif /* assetkit_obj_exp_writer_h */
