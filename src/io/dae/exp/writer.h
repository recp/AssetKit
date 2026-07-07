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

#ifndef assetkit_dae_exp_writer_h
#define assetkit_dae_exp_writer_h

#include "dae.h"
#include "../strpool.h"
#include "../../common/text_number.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define DAE_EXP_WRITER_CAP (64u * 1024u)
#define DAE_EXP_MAX_NODE_DEPTH 512u

typedef struct DAEExpWriter {
  FILE         *file;
  size_t        len;
  AkResult      result;
  unsigned char buffer[DAE_EXP_WRITER_CAP];
} DAEExpWriter;

typedef struct DAEExpName {
  const char *ptr;
  size_t      len;
} DAEExpName;

static inline
DAEExpName
dae_exp_name_make(const char * __restrict ptr, size_t len) {
  DAEExpName name;
  name.ptr = ptr;
  name.len = len;
  return name;
}

#define DAE_EXP_NAME(NAME) dae_exp_name_make(_s_dae_##NAME, _s_dae_##NAME##_len)
#define DAE_EXP_NAME_LIT(LIT) dae_exp_name_make((LIT), sizeof(LIT) - 1u)

static inline
DAEExpName
dae_exp_name_cstr(const char * __restrict str) {
  return dae_exp_name_make(str, str ? strlen(str) : 0u);
}

#define DAE_EXP_NAME_CSTR(STR) dae_exp_name_cstr((STR))

AK_HIDE
void
dae_w_flush(DAEExpWriter * __restrict w);

static inline
void
dae_w_ch(DAEExpWriter * __restrict w, char ch) {
  if (w->len == DAE_EXP_WRITER_CAP)
    dae_w_flush(w);

  w->buffer[w->len++] = (unsigned char)ch;
}

AK_HIDE
void
dae_w_raw(DAEExpWriter * __restrict w,
          const void   * __restrict data,
          size_t                    len);

AK_HIDE
void
dae_w_name(DAEExpWriter * __restrict w, DAEExpName name);

AK_HIDE
void
dae_w_cstr(DAEExpWriter * __restrict w, const char * __restrict str);

#define dae_w_lit(W, LIT) dae_w_raw((W), "" LIT, sizeof("" LIT) - 1u)

AK_INLINE
void
dae_w_uint_fast(DAEExpWriter * __restrict w, size_t val) {
  char *dst;
  char *end;

  if (DAE_EXP_WRITER_CAP - w->len < 24u)
    dae_w_flush(w);

  dst     = (char *)w->buffer + w->len;
  end     = ak_io_text_format_uint64(dst, (uint64_t)val);
  w->len += (size_t)(end - dst);
}

AK_INLINE
void
dae_w_float_fast(DAEExpWriter * __restrict w, float val) {
  char  *dst;
  size_t avail;
  size_t outLen;

  avail = DAE_EXP_WRITER_CAP - w->len;
  if (avail < 48u) {
    dae_w_flush(w);
    avail = DAE_EXP_WRITER_CAP - w->len;
  }

  dst = (char *)w->buffer + w->len;
  if (!ak_io_text_format_float9(dst, avail, val, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  w->len += outLen;
}

AK_HIDE
void
dae_w_double(DAEExpWriter * __restrict w, double val);

AK_HIDE
void
dae_w_xml(DAEExpWriter * __restrict w,
          const char   * __restrict str,
          bool                      attr);

AK_HIDE
void
dae_write_extra(DAEExpWriter * __restrict w, AkTreeNode * __restrict extra);

AK_HIDE
void
dae_w_attr_uint(DAEExpWriter * __restrict w,
                DAEExpName                name,
                size_t                    value);

AK_HIDE
void
dae_w_id(DAEExpWriter * __restrict w,
         DAEExpName                prefix,
         uint32_t                  idx);

#endif /* assetkit_dae_exp_writer_h */
