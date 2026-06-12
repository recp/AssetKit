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

#ifndef assetkit_dae_exp_io_h
#define assetkit_dae_exp_io_h

#include "dae.h"

#include <stdbool.h>
#include <stddef.h>

AK_HIDE
char*
dae_output_dir(const char * __restrict filepath);

AK_HIDE
bool
dae_path_is_abs(const char * __restrict path);

AK_HIDE
bool
dae_uri_has_scheme(const char * __restrict uri);

AK_HIDE
bool
dae_uri_is_data(const char * __restrict uri);

AK_HIDE
bool
dae_uri_is_file_scheme(const char * __restrict uri);

AK_HIDE
bool
dae_uri_decode_path(const char * __restrict uri,
                    char       * __restrict dst,
                    size_t                  dstCap);

AK_HIDE
const char*
dae_uri_file_path(const char * __restrict uri);

AK_HIDE
bool
dae_uri_rel_safe(const char * __restrict uri);

AK_HIDE
char*
dae_join_path(const char * __restrict dir, const char * __restrict rel);

AK_HIDE
bool
dae_join_path_buf(const char * __restrict dir,
                  const char * __restrict rel,
                  char       * __restrict path,
                  size_t                  pathCap);

AK_HIDE
bool
dae_mkdir_parent_dirs(char * __restrict path);

AK_HIDE
bool
dae_write_file_bytes(const char * __restrict dst,
                     const void * __restrict data,
                     size_t                  len);

AK_HIDE
char*
dae_strdup(const char * __restrict src);

#endif /* assetkit_dae_exp_io_h */
