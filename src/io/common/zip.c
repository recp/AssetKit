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
#include "../../miniz/miniz.h"
#include "../../utils.h"

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
  uint32_t crc32;
  uint32_t localOffset;
} AkZipStoredEntry;

static
uint16_t
ak_zip_read_u16le(const unsigned char * __restrict p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static
uint32_t
ak_zip_read_u32le(const unsigned char * __restrict p) {
  return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8u)
         | ((uint32_t)p[2] << 16u)
         | ((uint32_t)p[3] << 24u);
}

static
void
ak_zip_write_u16le(FILE * __restrict file, uint16_t value) {
  unsigned char out[2];

  out[0] = (unsigned char)(value & 0xffu);
  out[1] = (unsigned char)((value >> 8u) & 0xffu);
  (void)fwrite(out, 1, sizeof(out), file);
}

static
void
ak_zip_write_u32le(FILE * __restrict file, uint32_t value) {
  unsigned char out[4];

  out[0] = (unsigned char)(value & 0xffu);
  out[1] = (unsigned char)((value >> 8u) & 0xffu);
  out[2] = (unsigned char)((value >> 16u) & 0xffu);
  out[3] = (unsigned char)((value >> 24u) & 0xffu);
  (void)fwrite(out, 1, sizeof(out), file);
}

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
const unsigned char*
ak_zip_find_eocd(const unsigned char * __restrict data, size_t size) {
  size_t start;
  size_t i;

  if (!data || size < AK_ZIP_EOCD_MIN_SIZE)
    return NULL;

  start = size > AK_ZIP_EOCD_MAX_SEARCH ? size - AK_ZIP_EOCD_MAX_SEARCH : 0u;
  i     = size - AK_ZIP_EOCD_MIN_SIZE;

  for (;;) {
    if (ak_zip_read_u32le(data + i) == AK_ZIP_END_CENTRAL_DIRECTORY)
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
  if (ak_zip_read_u32le(p) != AK_ZIP_CENTRAL_FILE_HEADER)
    return false;

  entry->flags            = ak_zip_read_u16le(p + 8);
  entry->method           = ak_zip_read_u16le(p + 10);
  entry->crc32            = ak_zip_read_u32le(p + 16);
  entry->compressedSize   = ak_zip_read_u32le(p + 20);
  entry->uncompressedSize = ak_zip_read_u32le(p + 24);
  entry->nameLen          = ak_zip_read_u16le(p + 28);
  entry->extraLen         = ak_zip_read_u16le(p + 30);
  entry->commentLen       = ak_zip_read_u16le(p + 32);
  entry->localOffset      = ak_zip_read_u32le(p + 42);
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
ak_zip_extract_entry(const unsigned char       * __restrict zipData,
                     size_t                                 zipSize,
                     const AkZipCentralEntry   * __restrict entry,
                     void                     ** __restrict outData,
                     size_t                    * __restrict outSize) {
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
  if (ak_zip_read_u32le(local) != AK_ZIP_LOCAL_FILE_HEADER)
    return AK_EBADF;

  localNameLen  = ak_zip_read_u16le(local + 26);
  localExtraLen = ak_zip_read_u16le(local + 28);
  srcOffset     = localOffset + 30u + localNameLen + localExtraLen;
  srcSize       = entry->compressedSize;
  dstSize       = entry->uncompressedSize;

  if (srcOffset > zipSize || srcSize > zipSize - srcOffset)
    return AK_EBADF;

  dst = malloc(dstSize + 1u);
  if (!dst)
    return AK_ERR;

  src = zipData + srcOffset;
  if (entry->method == AK_ZIP_METHOD_STORED) {
    if (srcSize != dstSize) {
      free(dst);
      return AK_EBADF;
    }
    memcpy(dst, src, dstSize);
  } else if (entry->method == AK_ZIP_METHOD_DEFLATED) {
    size_t written;

    written = tinfl_decompress_mem_to_mem(dst, dstSize, src, srcSize, 0);
    if (written != dstSize) {
      free(dst);
      return AK_EBADF;
    }
  } else {
    free(dst);
    return AK_EBADF;
  }

  if (mz_crc32(MZ_CRC32_INIT, dst, dstSize) != entry->crc32) {
    free(dst);
    return AK_EBADF;
  }

  dst[dstSize] = '\0';
  *outData     = dst;
  *outSize     = dstSize;
  return AK_OK;
}

AK_HIDE
AkResult
ak_zip_visit_entries(const char        * __restrict zipPath,
                     AkZipEntryVisitor              visitor,
                     void             * __restrict userdata) {
  void                *fileData;
  size_t               fileSize;
  const unsigned char *data;
  const unsigned char *eocd;
  uint16_t             entryCount;
  uint32_t             centralOffset;
  uint32_t             centralSize;
  size_t               cursor;
  size_t               i;
  AkResult             result;

  if (!zipPath || !visitor)
    return AK_ERR;

  result = ak_readfile(zipPath, NULL, &fileData, &fileSize);
  if (result != AK_OK)
    return result;

  data = fileData;
  eocd = ak_zip_find_eocd(data, fileSize);
  if (!eocd) {
    ak_releasefile(fileData, fileSize);
    return AK_EBADF;
  }

  entryCount    = ak_zip_read_u16le(eocd + 10);
  centralSize   = ak_zip_read_u32le(eocd + 12);
  centralOffset = ak_zip_read_u32le(eocd + 16);

  if ((size_t)centralOffset > fileSize
      || (size_t)centralSize > fileSize - (size_t)centralOffset) {
    ak_releasefile(fileData, fileSize);
    return AK_EBADF;
  }

  cursor = centralOffset;
  result = AK_OK;
  for (i = 0; i < entryCount; i++) {
    AkZipCentralEntry entry;
    AkZipEntryInfo    info;

    if (!ak_zip_read_central_entry(data, fileSize, &cursor, &entry)) {
      result = AK_EBADF;
      break;
    }

    info.name             = (const char *)entry.name;
    info.nameLen          = entry.nameLen;
    info.compressedSize   = entry.compressedSize;
    info.uncompressedSize = entry.uncompressedSize;
    info.method           = entry.method;
    info.flags            = entry.flags;
    if (!visitor(&info, userdata))
      break;
  }

  ak_releasefile(fileData, fileSize);
  return result;
}

AK_HIDE
AkResult
ak_zip_extract_file(const char * __restrict zipPath,
                    const char * __restrict entryName,
                    void      ** __restrict outData,
                    size_t     * __restrict outSize) {
  void                *fileData;
  size_t               fileSize;
  const unsigned char *data;
  const unsigned char *eocd;
  uint16_t             entryCount;
  uint32_t             centralOffset;
  uint32_t             centralSize;
  size_t               cursor;
  size_t               i;
  AkResult             result;

  if (!zipPath || !entryName || !outData || !outSize)
    return AK_ERR;

  *outData = NULL;
  *outSize = 0;

  result = ak_readfile(zipPath, NULL, &fileData, &fileSize);
  if (result != AK_OK)
    return result;

  data = fileData;
  eocd = ak_zip_find_eocd(data, fileSize);
  if (!eocd) {
    ak_releasefile(fileData, fileSize);
    return AK_EBADF;
  }

  entryCount   = ak_zip_read_u16le(eocd + 10);
  centralSize  = ak_zip_read_u32le(eocd + 12);
  centralOffset = ak_zip_read_u32le(eocd + 16);

  if ((size_t)centralOffset > fileSize
      || (size_t)centralSize > fileSize - (size_t)centralOffset) {
    ak_releasefile(fileData, fileSize);
    return AK_EBADF;
  }

  cursor = centralOffset;
  result = AK_EBADF;
  for (i = 0; i < entryCount; i++) {
    AkZipCentralEntry entry;

    if (!ak_zip_read_central_entry(data, fileSize, &cursor, &entry)) {
      result = AK_EBADF;
      break;
    }

    if (!ak_zip_entry_name_eq(entry.name, entry.nameLen, entryName))
      continue;

    result = ak_zip_extract_entry(data, fileSize, &entry, outData, outSize);
    break;
  }

  ak_releasefile(fileData, fileSize);
  return result;
}

static
bool
ak_zip_write_local_file(FILE                   * __restrict file,
                        const AkZipWriteEntry * __restrict entry,
                        AkZipStoredEntry      * __restrict stored) {
  long nameLen;
  long offset;

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

  stored->crc32       = mz_crc32(MZ_CRC32_INIT, entry->data, entry->size);
  stored->localOffset = (uint32_t)offset;

  ak_zip_write_u32le(file, AK_ZIP_LOCAL_FILE_HEADER);
  ak_zip_write_u16le(file, 20u);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u16le(file, AK_ZIP_METHOD_STORED);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u32le(file, stored->crc32);
  ak_zip_write_u32le(file, (uint32_t)entry->size);
  ak_zip_write_u32le(file, (uint32_t)entry->size);
  ak_zip_write_u16le(file, (uint16_t)nameLen);
  ak_zip_write_u16le(file, 0u);

  return fwrite(entry->name, 1, (size_t)nameLen, file) == (size_t)nameLen
         && fwrite(entry->data, 1, entry->size, file) == entry->size;
}

static
bool
ak_zip_write_central_file(FILE                   * __restrict file,
                          const AkZipWriteEntry * __restrict entry,
                          const AkZipStoredEntry * __restrict stored) {
  size_t nameLen;

  nameLen = strlen(entry->name);
  if (nameLen == 0 || nameLen > UINT16_MAX || entry->size > UINT32_MAX)
    return false;

  ak_zip_write_u32le(file, AK_ZIP_CENTRAL_FILE_HEADER);
  ak_zip_write_u16le(file, 20u);
  ak_zip_write_u16le(file, 20u);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u16le(file, AK_ZIP_METHOD_STORED);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u32le(file, stored->crc32);
  ak_zip_write_u32le(file, (uint32_t)entry->size);
  ak_zip_write_u32le(file, (uint32_t)entry->size);
  ak_zip_write_u16le(file, (uint16_t)nameLen);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u16le(file, 0u);
  ak_zip_write_u32le(file, 0u);
  ak_zip_write_u32le(file, stored->localOffset);

  return fwrite(entry->name, 1, nameLen, file) == nameLen;
}

AK_HIDE
AkResult
ak_zip_write_stored(const char            * __restrict zipPath,
                    const AkZipWriteEntry * __restrict entries,
                    size_t                             entryCount) {
  AkZipStoredEntry *stored;
  FILE             *file;
  long              centralOffset;
  long              centralEnd;
  size_t            i;
  AkResult          result;

  if (!zipPath || !entries || entryCount == 0 || entryCount > UINT16_MAX)
    return AK_ERR;

  stored = calloc(entryCount, sizeof(*stored));
  if (!stored)
    return AK_ERR;

  file = fopen(zipPath, "wb");
  if (!file) {
    free(stored);
    return AK_EBADF;
  }

  result = AK_OK;
  for (i = 0; i < entryCount; i++) {
    if (!ak_zip_write_local_file(file, &entries[i], &stored[i])) {
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
    ak_zip_write_u32le(file, AK_ZIP_END_CENTRAL_DIRECTORY);
    ak_zip_write_u16le(file, 0u);
    ak_zip_write_u16le(file, 0u);
    ak_zip_write_u16le(file, (uint16_t)entryCount);
    ak_zip_write_u16le(file, (uint16_t)entryCount);
    ak_zip_write_u32le(file, (uint32_t)(centralEnd - centralOffset));
    ak_zip_write_u32le(file, (uint32_t)centralOffset);
    ak_zip_write_u16le(file, 0u);
  }

  if (fclose(file) != 0 && result == AK_OK)
    result = AK_ERR;

  free(stored);
  if (result != AK_OK)
    remove(zipPath);

  return result;
}
