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

#ifndef assetkit_gltf_exp_writer_h
#define assetkit_gltf_exp_writer_h

#include "../../../../include/ak/assetkit.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define GLTF_EXP_WRITER_CAP (64u * 1024u)

typedef struct GLTFExpWriter {
  FILE          *file;
  unsigned char *mem;
  size_t         memLen;
  size_t         memCap;
  size_t         len;
  AkResult       result;
  bool           memory;
  unsigned char  buffer[GLTF_EXP_WRITER_CAP];
} GLTFExpWriter;

bool
gltf_w_init_memory(GLTFExpWriter * __restrict w,
                   size_t                     capacity);

void
gltf_w_free(GLTFExpWriter * __restrict w);

void
gltf_w_flush(GLTFExpWriter * __restrict w);

void
gltf_w_raw(GLTFExpWriter * __restrict w,
           const void    * __restrict data,
           size_t                     len);

static inline
void
gltf_w_ch(GLTFExpWriter * __restrict w, char ch) {
  if (w->len == GLTF_EXP_WRITER_CAP)
    gltf_w_flush(w);

  w->buffer[w->len++] = (unsigned char)ch;
}

void
gltf_w_qstr_len(GLTFExpWriter * __restrict w,
                const char    * __restrict str,
                size_t                     len);

void
gltf_w_qstr(GLTFExpWriter * __restrict w,
            const char    * __restrict str);

void
gltf_w_key(GLTFExpWriter * __restrict w,
           const char    * __restrict key,
           size_t                     keyLen);

void
gltf_w_key_str(GLTFExpWriter * __restrict w,
               const char    * __restrict key,
               size_t                     keyLen,
               const char    * __restrict val);

void
gltf_w_uint(GLTFExpWriter * __restrict w, size_t val);

void
gltf_w_key_uint(GLTFExpWriter * __restrict w,
                const char    * __restrict key,
                size_t                     keyLen,
                size_t                     val);

void
gltf_w_key_bool(GLTFExpWriter * __restrict w,
                const char    * __restrict key,
                size_t                     keyLen,
                bool                       val);

void
gltf_w_float(GLTFExpWriter * __restrict w, float val);

void
gltf_w_number(GLTFExpWriter * __restrict w, double val);

#endif /* assetkit_gltf_exp_writer_h */
