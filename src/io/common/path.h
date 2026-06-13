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

#ifndef io_common_path_h
#define io_common_path_h

#include "../../common.h"

#include <stdbool.h>
#include <stddef.h>

AK_HIDE
char*
io_path_output_dir_dup(const char * __restrict filepath);

AK_HIDE
const char*
io_path_basename(const char * __restrict path);

AK_HIDE
const char*
io_path_basename_nonempty(const char * __restrict path);

AK_HIDE
char*
io_path_basename_dup(const char * __restrict path);

AK_HIDE
char*
io_path_replace_extension_dup(const char * __restrict filepath,
                              const char * __restrict ext,
                              size_t                  extLen);

AK_HIDE
bool
io_path_join_parts(const char * __restrict dir,
                   const char * __restrict rel,
                   size_t     * __restrict dirLen,
                   size_t     * __restrict relLen,
                   bool       * __restrict sep,
                   size_t     * __restrict need);

AK_HIDE
void
io_path_join_write(char       * __restrict path,
                   const char * __restrict dir,
                   const char * __restrict rel,
                   size_t                  dirLen,
                   size_t                  relLen,
                   bool                    sep);

AK_HIDE
char*
io_path_join_dup(const char * __restrict dir,
                 const char * __restrict rel);

AK_HIDE
char*
io_path_join_dup_trim_dir(const char * __restrict dir,
                          const char * __restrict rel);

AK_HIDE
bool
io_path_join_buf(const char * __restrict dir,
                 const char * __restrict rel,
                 char       * __restrict path,
                 size_t                  pathCap);

AK_HIDE
bool
io_path_mkdir_parent_dirs(char * __restrict path,
                          bool              normalizeSeparators);

#endif /* io_common_path_h */
