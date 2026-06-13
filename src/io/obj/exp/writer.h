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

AK_HIDE
void
wobj_w_flush(WOBJExpWriter * __restrict w);

AK_HIDE
void
wobj_w_raw(WOBJExpWriter * __restrict w,
           const void    * __restrict data,
           size_t                     len);

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

AK_HIDE
void
wobj_w_name(WOBJExpWriter * __restrict w,
            const char    * __restrict name);

AK_HIDE
void
wobj_w_uint(WOBJExpWriter * __restrict w, uint32_t val);

AK_HIDE
void
wobj_w_float(WOBJExpWriter * __restrict w, float val);

#endif /* assetkit_obj_exp_writer_h */
