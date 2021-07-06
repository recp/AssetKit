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

#ifndef assetkit_path_h
#define assetkit_path_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <stdio.h>

struct AkHeap;
struct AkDoc;

AK_EXPORT
const char *
ak_path_fragment(const char *path);

AK_EXPORT
size_t
ak_path_trim(const char *path,
             char *trimmed);

AK_EXPORT
int
ak_path_join(char   *fragments[],
             char   *buf,
             size_t *size);

AK_EXPORT
int
ak_path_isfile(const char *path);

AK_EXPORT
char*
ak_path_dir(struct AkHeap * __restrict heap,
            void          * __restrict memparent,
            const char    * __restrict path);

AK_EXPORT
const char*
ak_fullpath(struct AkDoc * __restrict doc,
            const char   * __restrict ref,
            char         * __restrict buf);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_path_h */
