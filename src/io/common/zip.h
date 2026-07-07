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

typedef struct AkZipArchive AkZipArchive;
typedef struct AkZipDecompressor AkZipDecompressor;

typedef struct AkZipWriteEntry {
  const char          *name;
  const void          *data;
  const AkZipArchive  *sourceArchive;
  size_t               size;
  size_t               sourceIndex;
} AkZipWriteEntry;

typedef struct AkZipEntryInfo {
  const char    *name;
  size_t         index;
  size_t         nameLen;
  uint32_t       compressedSize;
  uint32_t       uncompressedSize;
  uint16_t       method;
  uint16_t       flags;
} AkZipEntryInfo;

typedef bool (*AkZipEntryVisitor)(const AkZipEntryInfo * __restrict info,
                                  void                 * __restrict userdata);
typedef void *(*AkZipAllocFn)(void * __restrict userdata, size_t size);
typedef void (*AkZipFreeFn)(void * __restrict userdata, void * __restrict data);

AK_HIDE
AkResult
ak_zip_open(const char      * __restrict zipPath,
            AkZipArchive   ** __restrict archive);

AK_HIDE
void
ak_zip_close(AkZipArchive * __restrict archive);

AK_HIDE
AkZipDecompressor*
ak_zip_decompressor_new(void);

AK_HIDE
void
ak_zip_decompressor_free(AkZipDecompressor * __restrict decompressor);

AK_HIDE
AkResult
ak_zip_archive_visit_entries(AkZipArchive      * __restrict archive,
                             AkZipEntryVisitor              visitor,
                             void             * __restrict userdata);

AK_HIDE
bool
ak_zip_archive_find_entry_index(AkZipArchive * __restrict archive,
                                const char   * __restrict entryName,
                                size_t       * __restrict outIndex);

AK_HIDE
AkResult
ak_zip_archive_extract_file(AkZipArchive * __restrict archive,
                            const char   * __restrict entryName,
                            void        ** __restrict outData,
                            size_t       * __restrict outSize);

AK_HIDE
AkResult
ak_zip_archive_extract_file_alloc(AkZipArchive * __restrict archive,
                                  const char   * __restrict entryName,
                                  AkZipAllocFn               allocFn,
                                  AkZipFreeFn                freeFn,
                                  void        * __restrict allocUserdata,
                                  void       ** __restrict outData,
                                  size_t      * __restrict outSize);

AK_HIDE
AkResult
ak_zip_archive_extract_index_to(AkZipArchive       * __restrict archive,
                                size_t                          entryIndex,
                                AkZipDecompressor * __restrict decompressor,
                                void              * __restrict outData,
                                size_t                         outCapacity,
                                size_t            * __restrict outSize);

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

AK_HIDE
AkResult
ak_zip_write_deflated_level(const char            * __restrict zipPath,
                            const AkZipWriteEntry * __restrict entries,
                            size_t                             entryCount,
                            unsigned                           compressionLevel);

#endif /* assetkit_io_common_zip_h */
