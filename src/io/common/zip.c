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

#include "zip.h"
#include "binary.h"
#include "../../utils.h"

#include <libdeflate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AK_ZIP_LOCAL_FILE_HEADER      0x04034b50u
#define AK_ZIP_CENTRAL_FILE_HEADER    0x02014b50u
#define AK_ZIP_END_CENTRAL_DIRECTORY  0x06054b50u
#define AK_ZIP_METHOD_STORED          0u
#define AK_ZIP_METHOD_DEFLATED        8u
#define AK_ZIP_EOCD_MIN_SIZE          22u
#define AK_ZIP_EOCD_MAX_SEARCH        (0xffffu + AK_ZIP_EOCD_MIN_SIZE)

typedef struct AkZipCentralEntry {
  uint32_t compressedSize;
  uint32_t uncompressedSize;
  uint32_t crc32;
  uint32_t localOffset;
  uint16_t method;
  uint16_t flags;
  uint16_t nameLen;
  uint16_t extraLen;
  uint16_t commentLen;
  const unsigned char *name;
} AkZipCentralEntry;

typedef struct AkZipStoredEntry {
  uint32_t compressedSize;
  uint32_t uncompressedSize;
  uint32_t crc32;
  uint32_t localOffset;
  uint16_t method;
  uint16_t nameLen;
} AkZipStoredEntry;

struct AkZipDecompressor {
  struct libdeflate_decompressor *impl;
};

struct AkZipArchive {
  void                         *fileData;
  size_t                        fileSize;
  AkZipCentralEntry            *entries;
  uint16_t                      entryCount;
  AkZipDecompressor            *decompressor;
};

static
bool
ak_zip_entry_name_eq(const unsigned char * __restrict name,
                     uint16_t                         nameLen,
                     const char          * __restrict want) {
  size_t wantLen;

  if (!name || !want)
    return false;

  while (*want == '/')
    want++;

  wantLen = strlen(want);
  return wantLen == nameLen && memcmp(name, want, nameLen) == 0;
}

static
void*
ak_zip_default_alloc(void * __restrict userdata, size_t size) {
  (void)userdata;
  return malloc(size);
}

static
void
ak_zip_default_free(void * __restrict userdata, void * __restrict data) {
  (void)userdata;
  free(data);
}

static
const unsigned char*
ak_zip_find_eocd(const unsigned char * __restrict data, size_t size) {
  size_t start;
  size_t i;

  if (!data || size < AK_ZIP_EOCD_MIN_SIZE)
    return NULL;

  start = size > AK_ZIP_EOCD_MAX_SEARCH ? size - AK_ZIP_EOCD_MAX_SEARCH : 0u;
  i     = size - AK_ZIP_EOCD_MIN_SIZE;

  for (;;) {
    if (io_load_u32le(data + i) == AK_ZIP_END_CENTRAL_DIRECTORY)
      return data + i;
    if (i == start)
      break;
    i--;
  }

  return NULL;
}

static
bool
ak_zip_read_central_entry(const unsigned char * __restrict data,
                          size_t                           size,
                          size_t                 * __restrict cursor,
                          AkZipCentralEntry      * __restrict entry) {
  const unsigned char *p;
  size_t              pos;

  pos = *cursor;
  if (pos > size || size - pos < 46u)
    return false;

  p = data + pos;
  if (io_load_u32le(p) != AK_ZIP_CENTRAL_FILE_HEADER)
    return false;

  entry->flags            = io_load_u16le(p + 8);
  entry->method           = io_load_u16le(p + 10);
  entry->crc32            = io_load_u32le(p + 16);
  entry->compressedSize   = io_load_u32le(p + 20);
  entry->uncompressedSize = io_load_u32le(p + 24);
  entry->nameLen          = io_load_u16le(p + 28);
  entry->extraLen         = io_load_u16le(p + 30);
  entry->commentLen       = io_load_u16le(p + 32);
  entry->localOffset      = io_load_u32le(p + 42);
  entry->name             = p + 46;

  pos += 46u;
  if (entry->nameLen > size - pos)
    return false;
  pos += entry->nameLen;
  if (entry->extraLen > size - pos)
    return false;
  pos += entry->extraLen;
  if (entry->commentLen > size - pos)
    return false;
  pos += entry->commentLen;

  *cursor = pos;
  return true;
}

static
AkResult
ak_zip_extract_entry_to(const unsigned char     * __restrict zipData,
                        size_t                               zipSize,
                        const AkZipCentralEntry * __restrict entry,
                        AkZipDecompressor       * __restrict decompressor,
                        void                    * __restrict outData,
                        size_t                               outCapacity,
                        size_t                  * __restrict outSize) {
  const unsigned char *local;
  const unsigned char *src;
  unsigned char       *dst;
  size_t               localOffset;
  size_t               srcOffset;
  size_t               srcSize;
  size_t               dstSize;
  uint16_t             localNameLen;
  uint16_t             localExtraLen;

  if (!zipData || !entry || !outData || !outSize)
    return AK_ERR;

  localOffset = entry->localOffset;
  if (localOffset > zipSize || zipSize - localOffset < 30u)
    return AK_EBADF;

  local = zipData + localOffset;
  if (io_load_u32le(local) != AK_ZIP_LOCAL_FILE_HEADER)
    return AK_EBADF;

  localNameLen  = io_load_u16le(local + 26);
  localExtraLen = io_load_u16le(local + 28);
  srcOffset     = localOffset + 30u + localNameLen + localExtraLen;
  srcSize       = entry->compressedSize;
  dstSize       = entry->uncompressedSize;

  if (srcOffset > zipSize || srcSize > zipSize - srcOffset)
    return AK_EBADF;
  if (outCapacity <= dstSize)
    return AK_ERR;

  dst = outData;
  src = zipData + srcOffset;
  if (entry->method == AK_ZIP_METHOD_STORED) {
    if (srcSize != dstSize)
      return AK_EBADF;
    memcpy(dst, src, dstSize);
  } else if (entry->method == AK_ZIP_METHOD_DEFLATED) {
    enum libdeflate_result          decompResult;
    size_t written;

    if (!decompressor || !decompressor->impl)
      return AK_ERR;

    written      = 0u;
    decompResult = libdeflate_deflate_decompress(decompressor->impl,
                                                 src,
                                                 srcSize,
                                                 dst,
                                                 dstSize,
                                                 &written);
    if (decompResult != LIBDEFLATE_SUCCESS || written != dstSize)
      return AK_EBADF;
  } else {
    return AK_EBADF;
  }

  if (libdeflate_crc32(0u, dst, dstSize) != entry->crc32)
    return AK_EBADF;

  dst[dstSize] = '\0';
  *outSize     = dstSize;
  return AK_OK;
}

static
AkResult
ak_zip_extract_entry(const unsigned char     * __restrict zipData,
                     size_t                               zipSize,
                     const AkZipCentralEntry * __restrict entry,
                     AkZipDecompressor       * __restrict decompressor,
                     AkZipAllocFn                         allocFn,
                     AkZipFreeFn                          freeFn,
                     void                    * __restrict allocUserdata,
                     void                   ** __restrict outData,
                     size_t                  * __restrict outSize) {
  void     *dst;
  size_t    dstSize;
  AkResult  result;

  if (!zipData || !entry || !outData || !outSize)
    return AK_ERR;
  if (!allocFn)
    allocFn = ak_zip_default_alloc;
  if (!freeFn)
    freeFn = ak_zip_default_free;

  dstSize = entry->uncompressedSize;
  dst     = allocFn(allocUserdata, dstSize + 1u);
  if (!dst)
    return AK_ERR;

  result = ak_zip_extract_entry_to(zipData,
                                   zipSize,
                                   entry,
                                   decompressor,
                                   dst,
                                   dstSize + 1u,
                                   outSize);
  if (result != AK_OK) {
    freeFn(allocUserdata, dst);
    return result;
  }

  *outData = dst;
  return AK_OK;
}

AK_HIDE
AkResult
ak_zip_open(const char      * __restrict zipPath,
            AkZipArchive   ** __restrict archive) {
  AkZipArchive       *zip;
  const unsigned char *data;
  const unsigned char *eocd;
  uint16_t             entryCount;
  uint32_t             centralOffset;
  uint32_t             centralSize;
  size_t               cursor;
  size_t               i;
  AkResult             result;

  if (!zipPath || !archive)
    return AK_ERR;

  *archive = NULL;
  zip = calloc(1u, sizeof(*zip));
  if (!zip)
    return AK_ERR;

  result = ak_readfile(zipPath, NULL, &zip->fileData, &zip->fileSize);
  if (result != AK_OK)
    goto fail;

  data = zip->fileData;
  eocd = ak_zip_find_eocd(data, zip->fileSize);
  if (!eocd) {
    result = AK_EBADF;
    goto fail;
  }

  entryCount    = io_load_u16le(eocd + 10);
  centralSize   = io_load_u32le(eocd + 12);
  centralOffset = io_load_u32le(eocd + 16);

  if ((size_t)centralOffset > zip->fileSize
      || (size_t)centralSize > zip->fileSize - (size_t)centralOffset) {
    result = AK_EBADF;
    goto fail;
  }

  if (entryCount > 0u) {
    zip->entries = calloc(entryCount, sizeof(*zip->entries));
    if (!zip->entries) {
      result = AK_ERR;
      goto fail;
    }
  }

  cursor = centralOffset;
  for (i = 0; i < entryCount; i++) {
    if (!ak_zip_read_central_entry(data, zip->fileSize, &cursor, &zip->entries[i])) {
      result = AK_EBADF;
      goto fail;
    }
  }

  zip->decompressor = ak_zip_decompressor_new();
  if (!zip->decompressor) {
    result = AK_ERR;
    goto fail;
  }

  zip->entryCount = entryCount;
  *archive        = zip;
  return AK_OK;

fail:
  ak_zip_close(zip);
  return result;
}

AK_HIDE
void
ak_zip_close(AkZipArchive * __restrict archive) {
  if (!archive)
    return;

  if (archive->decompressor)
    ak_zip_decompressor_free(archive->decompressor);
  free(archive->entries);
  if (archive->fileData)
    ak_releasefile(archive->fileData, archive->fileSize);
  free(archive);
}

AK_HIDE
AkZipDecompressor*
ak_zip_decompressor_new(void) {
  AkZipDecompressor *decompressor;

  decompressor = calloc(1u, sizeof(*decompressor));
  if (!decompressor)
    return NULL;

  decompressor->impl = libdeflate_alloc_decompressor();
  if (!decompressor->impl) {
    free(decompressor);
    return NULL;
  }

  return decompressor;
}

AK_HIDE
void
ak_zip_decompressor_free(AkZipDecompressor * __restrict decompressor) {
  if (!decompressor)
    return;

  if (decompressor->impl)
    libdeflate_free_decompressor(decompressor->impl);
  free(decompressor);
}

static
const AkZipCentralEntry*
ak_zip_archive_find_entry(AkZipArchive * __restrict archive,
                          const char   * __restrict entryName) {
  size_t i;

  if (!archive || !entryName)
    return NULL;

  for (i = 0u; i < archive->entryCount; i++) {
    const AkZipCentralEntry *entry;

    entry = &archive->entries[i];
    if (ak_zip_entry_name_eq(entry->name, entry->nameLen, entryName))
      return entry;
  }

  return NULL;
}

AK_HIDE
AkResult
ak_zip_archive_visit_entries(AkZipArchive      * __restrict archive,
                             AkZipEntryVisitor              visitor,
                             void             * __restrict userdata) {
  size_t i;

  if (!archive || !visitor)
    return AK_ERR;

  for (i = 0u; i < archive->entryCount; i++) {
    const AkZipCentralEntry *entry;
    AkZipEntryInfo    info;

    entry                 = &archive->entries[i];
    info.name             = (const char *)entry->name;
    info.index            = i;
    info.nameLen          = entry->nameLen;
    info.compressedSize   = entry->compressedSize;
    info.uncompressedSize = entry->uncompressedSize;
    info.method           = entry->method;
    info.flags            = entry->flags;
    if (!visitor(&info, userdata))
      break;
  }

  return AK_OK;
}

AK_HIDE
AkResult
ak_zip_archive_extract_file(AkZipArchive * __restrict archive,
                            const char   * __restrict entryName,
                            void        ** __restrict outData,
                            size_t       * __restrict outSize) {
  return ak_zip_archive_extract_file_alloc(archive,
                                           entryName,
                                           NULL,
                                           NULL,
                                           NULL,
                                           outData,
                                           outSize);
}

AK_HIDE
AkResult
ak_zip_archive_extract_file_alloc(AkZipArchive * __restrict archive,
                                  const char   * __restrict entryName,
                                  AkZipAllocFn               allocFn,
                                  AkZipFreeFn                freeFn,
                                  void        * __restrict allocUserdata,
                                  void       ** __restrict outData,
                                  size_t      * __restrict outSize) {
  const AkZipCentralEntry *entry;

  if (!archive || !entryName || !outData || !outSize)
    return AK_ERR;

  *outData = NULL;
  *outSize = 0u;
  entry = ak_zip_archive_find_entry(archive, entryName);
  if (!entry)
    return AK_EBADF;

  return ak_zip_extract_entry(archive->fileData,
                              archive->fileSize,
                              entry,
                              archive->decompressor,
                              allocFn,
                              freeFn,
                              allocUserdata,
                              outData,
                              outSize);
}

AK_HIDE
AkResult
ak_zip_archive_extract_index_to(AkZipArchive       * __restrict archive,
                                size_t                          entryIndex,
                                AkZipDecompressor * __restrict decompressor,
                                void              * __restrict outData,
                                size_t                         outCapacity,
                                size_t            * __restrict outSize) {
  if (!archive || !outData || !outSize)
    return AK_ERR;
  if (entryIndex >= archive->entryCount)
    return AK_EBADF;

  *outSize = 0u;
  return ak_zip_extract_entry_to(archive->fileData,
                                 archive->fileSize,
                                 &archive->entries[entryIndex],
                                 decompressor ? decompressor : archive->decompressor,
                                 outData,
                                 outCapacity,
                                 outSize);
}

AK_HIDE
AkResult
ak_zip_visit_entries(const char        * __restrict zipPath,
                     AkZipEntryVisitor              visitor,
                     void             * __restrict userdata) {
  AkZipArchive *archive;
  AkResult      result;

  if (!zipPath || !visitor)
    return AK_ERR;

  archive = NULL;
  result  = ak_zip_open(zipPath, &archive);
  if (result == AK_OK)
    result = ak_zip_archive_visit_entries(archive, visitor, userdata);
  ak_zip_close(archive);
  return result;
}

AK_HIDE
AkResult
ak_zip_extract_file(const char * __restrict zipPath,
                    const char * __restrict entryName,
                    void      ** __restrict outData,
                    size_t     * __restrict outSize) {
  AkZipArchive *archive;
  AkResult      result;

  if (!zipPath || !entryName || !outData || !outSize)
    return AK_ERR;

  *outData = NULL;
  *outSize = 0;

  archive = NULL;
  result  = ak_zip_open(zipPath, &archive);
  if (result == AK_OK)
    result = ak_zip_archive_extract_file(archive, entryName, outData, outSize);
  ak_zip_close(archive);
  return result;
}

static
bool
ak_zip_write_local_header(FILE     * __restrict file,
                          uint16_t              nameLen,
                          uint16_t              method,
                          uint32_t              crc32,
                          uint32_t              compressedSize,
                          uint32_t              uncompressedSize) {
  unsigned char out[30];

  io_store_u32le(out + 0, AK_ZIP_LOCAL_FILE_HEADER);
  io_store_u16le(out + 4, 20u);
  io_store_u16le(out + 6, 0u);
  io_store_u16le(out + 8, method);
  io_store_u16le(out + 10, 0u);
  io_store_u16le(out + 12, 0u);
  io_store_u32le(out + 14, crc32);
  io_store_u32le(out + 18, compressedSize);
  io_store_u32le(out + 22, uncompressedSize);
  io_store_u16le(out + 26, nameLen);
  io_store_u16le(out + 28, 0u);

  return fwrite(out, 1, sizeof(out), file) == sizeof(out);
}

static
bool
ak_zip_write_central_header(FILE     * __restrict file,
                            uint16_t              nameLen,
                            uint16_t              method,
                            uint32_t              crc32,
                            uint32_t              compressedSize,
                            uint32_t              uncompressedSize,
                            uint32_t              localOffset) {
  unsigned char out[46];

  io_store_u32le(out + 0, AK_ZIP_CENTRAL_FILE_HEADER);
  io_store_u16le(out + 4, 20u);
  io_store_u16le(out + 6, 20u);
  io_store_u16le(out + 8, 0u);
  io_store_u16le(out + 10, method);
  io_store_u16le(out + 12, 0u);
  io_store_u16le(out + 14, 0u);
  io_store_u32le(out + 16, crc32);
  io_store_u32le(out + 20, compressedSize);
  io_store_u32le(out + 24, uncompressedSize);
  io_store_u16le(out + 28, nameLen);
  io_store_u16le(out + 30, 0u);
  io_store_u16le(out + 32, 0u);
  io_store_u16le(out + 34, 0u);
  io_store_u16le(out + 36, 0u);
  io_store_u32le(out + 38, 0u);
  io_store_u32le(out + 42, localOffset);

  return fwrite(out, 1, sizeof(out), file) == sizeof(out);
}

static
bool
ak_zip_write_eocd(FILE     * __restrict file,
                  uint16_t              entryCount,
                  uint32_t              centralSize,
                  uint32_t              centralOffset) {
  unsigned char out[22];

  io_store_u32le(out + 0, AK_ZIP_END_CENTRAL_DIRECTORY);
  io_store_u16le(out + 4, 0u);
  io_store_u16le(out + 6, 0u);
  io_store_u16le(out + 8, entryCount);
  io_store_u16le(out + 10, entryCount);
  io_store_u32le(out + 12, centralSize);
  io_store_u32le(out + 16, centralOffset);
  io_store_u16le(out + 20, 0u);

  return fwrite(out, 1, sizeof(out), file) == sizeof(out);
}

static
bool
ak_zip_reserve_scratch(unsigned char ** __restrict scratch,
                       size_t         * __restrict scratchCapacity,
                       size_t                      needed) {
  unsigned char *newScratch;

  if (needed == 0u || *scratchCapacity >= needed)
    return true;

  newScratch = realloc(*scratch, needed);
  if (!newScratch)
    return false;

  *scratch         = newScratch;
  *scratchCapacity = needed;
  return true;
}

static
bool
ak_zip_write_local_file(FILE                          * __restrict file,
                        const AkZipWriteEntry        * __restrict entry,
                        AkZipStoredEntry             * __restrict stored,
                        struct libdeflate_compressor * __restrict compressor,
                        unsigned char               ** __restrict scratch,
                        size_t                       * __restrict scratchCapacity) {
  const void *payload;
  size_t      payloadSize;
  size_t      compressedSize;
  size_t      maxCompressedSize;
  long        nameLen;
  long        offset;

  if (!file || !entry || !entry->name || (!entry->data && entry->size > 0u))
    return false;
  if (entry->size > UINT32_MAX)
    return false;

  nameLen = (long)strlen(entry->name);
  if (nameLen <= 0 || nameLen > UINT16_MAX)
    return false;

  offset = ftell(file);
  if (offset < 0 || offset > UINT32_MAX)
    return false;

  stored->crc32            = libdeflate_crc32(0u, entry->data, entry->size);
  stored->localOffset      = (uint32_t)offset;
  stored->nameLen          = (uint16_t)nameLen;
  stored->method           = AK_ZIP_METHOD_STORED;
  stored->compressedSize   = (uint32_t)entry->size;
  stored->uncompressedSize = (uint32_t)entry->size;
  payload                  = entry->data;
  payloadSize              = entry->size;

  if (compressor && entry->size > 1u) {
    maxCompressedSize = entry->size - 1u;
    if (!ak_zip_reserve_scratch(scratch, scratchCapacity, maxCompressedSize))
      return false;

    compressedSize = libdeflate_deflate_compress(compressor,
                                                 entry->data,
                                                 entry->size,
                                                 *scratch,
                                                 maxCompressedSize);
    if (compressedSize > 0u) {
      stored->method         = AK_ZIP_METHOD_DEFLATED;
      stored->compressedSize = (uint32_t)compressedSize;
      payload                = *scratch;
      payloadSize            = compressedSize;
    }
  }

  return ak_zip_write_local_header(file,
                                   stored->nameLen,
                                   stored->method,
                                   stored->crc32,
                                   stored->compressedSize,
                                   stored->uncompressedSize)
         && fwrite(entry->name, 1, (size_t)nameLen, file) == (size_t)nameLen
         && fwrite(payload, 1, payloadSize, file) == payloadSize;
}

static
bool
ak_zip_write_central_file(FILE                    * __restrict file,
                          const AkZipWriteEntry  * __restrict entry,
                          const AkZipStoredEntry * __restrict stored) {
  if (stored->nameLen == 0u || entry->size > UINT32_MAX)
    return false;

  return ak_zip_write_central_header(file,
                                     stored->nameLen,
                                     stored->method,
                                     stored->crc32,
                                     stored->compressedSize,
                                     stored->uncompressedSize,
                                     stored->localOffset)
         && fwrite(entry->name, 1, stored->nameLen, file) == stored->nameLen;
}

static
AkResult
ak_zip_write_entries(const char            * __restrict zipPath,
                     const AkZipWriteEntry * __restrict entries,
                     size_t                             entryCount,
                     unsigned                           compressionLevel) {
  AkZipStoredEntry             *stored;
  struct libdeflate_compressor *compressor;
  unsigned char                *scratch;
  FILE                         *file;
  long                          centralOffset;
  long                          centralEnd;
  size_t                        i;
  size_t                        scratchCapacity;
  AkResult                      result;

  if (!zipPath || !entries || entryCount == 0 || entryCount > UINT16_MAX)
    return AK_ERR;

  stored = calloc(entryCount, sizeof(*stored));
  if (!stored)
    return AK_ERR;

  compressor      = NULL;
  scratch         = NULL;
  scratchCapacity = 0u;
  if (compressionLevel > 12u)
    compressionLevel = 12u;

  if (compressionLevel > 0u) {
    compressor = libdeflate_alloc_compressor((int)compressionLevel);
    if (!compressor) {
      free(stored);
      return AK_ERR;
    }
  }

  file = fopen(zipPath, "wb");
  if (!file) {
    libdeflate_free_compressor(compressor);
    free(stored);
    return AK_EBADF;
  }

  result = AK_OK;
  for (i = 0; i < entryCount; i++) {
    if (!ak_zip_write_local_file(file,
                                 &entries[i],
                                 &stored[i],
                                 compressor,
                                 &scratch,
                                 &scratchCapacity)) {
      result = AK_ERR;
      break;
    }
  }

  centralOffset = ftell(file);
  if (centralOffset < 0 || centralOffset > UINT32_MAX)
    result = AK_ERR;

  if (result == AK_OK) {
    for (i = 0; i < entryCount; i++) {
      if (!ak_zip_write_central_file(file, &entries[i], &stored[i])) {
        result = AK_ERR;
        break;
      }
    }
  }

  centralEnd = ftell(file);
  if (centralEnd < 0
      || centralEnd < centralOffset
      || centralEnd - centralOffset > UINT32_MAX)
    result = AK_ERR;

  if (result == AK_OK) {
    if (!ak_zip_write_eocd(file,
                           (uint16_t)entryCount,
                           (uint32_t)(centralEnd - centralOffset),
                           (uint32_t)centralOffset))
      result = AK_ERR;
  }

  if (fclose(file) != 0 && result == AK_OK)
    result = AK_ERR;

  libdeflate_free_compressor(compressor);
  free(scratch);
  free(stored);
  if (result != AK_OK)
    remove(zipPath);

  return result;
}

AK_HIDE
AkResult
ak_zip_write_stored(const char            * __restrict zipPath,
                    const AkZipWriteEntry * __restrict entries,
                    size_t                             entryCount) {
  return ak_zip_write_entries(zipPath, entries, entryCount, 0u);
}

AK_HIDE
AkResult
ak_zip_write_deflated(const char            * __restrict zipPath,
                      const AkZipWriteEntry * __restrict entries,
                      size_t                             entryCount) {
  return ak_zip_write_entries(zipPath, entries, entryCount, 1u);
}

AK_HIDE
AkResult
ak_zip_write_deflated_level(const char            * __restrict zipPath,
                            const AkZipWriteEntry * __restrict entries,
                            size_t                             entryCount,
                            unsigned                           compressionLevel) {
  return ak_zip_write_entries(zipPath, entries, entryCount, compressionLevel);
}
