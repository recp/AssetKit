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

#ifndef dae_h
#define dae_h

#include "../../../include/ak/assetkit.h"

#include <stdlib.h>

struct AkZipArchive;

AK_HIDE
AkResult
dae_doc(AkDoc     ** __restrict dest,
        const char * __restrict filepath);

AK_HIDE
AkResult
dae_doc_memory(AkDoc     ** __restrict dest,
               const char * __restrict filepath,
               void       * __restrict xmlData,
               size_t                  xmlSize);

AK_HIDE
AkResult
dae_archive_doc(AkDoc     ** __restrict dest,
                const char * __restrict filepath);

AK_HIDE
AkResult
dae_archive_doc_archive(AkDoc              ** __restrict dest,
                        const char          * __restrict filepath,
                        struct AkZipArchive * __restrict archive,
                        const char          * __restrict entryName);

#endif /* dae_h */
