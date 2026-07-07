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
    WOBJ_W_LIT(w, "unnamed");
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
