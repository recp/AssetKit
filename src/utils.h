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

#ifndef ak_src_utils_h
#define ak_src_utils_h

#include "../include/ak/assetkit.h"
#include "../include/ak/util.h"

AkResult
ak_readfile(const char * __restrict file,
            void       * __restrict parent,
            void      ** __restrict dest,
            size_t     * __restrict size);
time_t
ak_parse_date(const char * __restrict input,
              const char ** __restrict ret);
AK_HIDE
void
ak_releasefile(void *file, size_t size);

#endif /* ak_src_utils_h */
