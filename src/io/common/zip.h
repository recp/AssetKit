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

#ifndef assetkit_io_common_zip_h
#define assetkit_io_common_zip_h

#include "../../common.h"

#include <stddef.h>
#include <stdint.h>

typedef struct AkZipWriteEntry {
  const char    *name;
  const void    *data;
  size_t         size;
} AkZipWriteEntry;

typedef struct AkZipEntryInfo {
  const char    *name;
  size_t         nameLen;
  uint32_t       compressedSize;
  uint32_t       uncompressedSize;
  uint16_t       method;
  uint16_t       flags;
} AkZipEntryInfo;

typedef bool (*AkZipEntryVisitor)(const AkZipEntryInfo * __restrict info,
                                  void                 * __restrict userdata);

AK_HIDE
AkResult
ak_zip_visit_entries(const char        * __restrict zipPath,
                     AkZipEntryVisitor              visitor,
                     void             * __restrict userdata);

AK_HIDE
AkResult
ak_zip_extract_file(const char * __restrict zipPath,
                    const char * __restrict entryName,
                    void      ** __restrict outData,
                    size_t     * __restrict outSize);

AK_HIDE
AkResult
ak_zip_write_stored(const char            * __restrict zipPath,
                    const AkZipWriteEntry * __restrict entries,
                    size_t                             entryCount);

AK_HIDE
AkResult
ak_zip_write_deflated(const char            * __restrict zipPath,
                      const AkZipWriteEntry * __restrict entries,
                      size_t                             entryCount);

#endif /* assetkit_io_common_zip_h */
