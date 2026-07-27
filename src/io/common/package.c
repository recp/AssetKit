/*
 * Copyright (C) 2026 Recep Aslantas
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

#include "package.h"
#include "path.h"
#include "zip.h"
#include "../dae/dae.h"
#include "../../image/export.h"
#include "../../resc/file_scope.h"
#include "../../utils.h"
#include "../../../include/ak/path.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(AK_WINAPI)
#  include <direct.h>
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

#define AK_PACKAGE_ZIP_METHOD_STORED   0u
#define AK_PACKAGE_ZIP_METHOD_DEFLATED 8u

typedef struct AkPackageEntry {
  char    *name;
  size_t   index;
  uint32_t uncompressedSize;
  uint16_t method;
  uint16_t flags;
} AkPackageEntry;

typedef struct AkPackageEntries {
  AkPackageEntry *items;
  char           *storage;
  char           *cursor;
  size_t          count;
  size_t          chars;
  size_t          next;
  bool            overflow;
} AkPackageEntries;

static
bool
ak_package_measure_entry(const AkZipEntryInfo * __restrict info,
                         void                 * __restrict userdata) {
  AkPackageEntries *entries;

  entries = userdata;
  if (entries->count == SIZE_MAX
      || info->nameLen == SIZE_MAX
      || entries->chars > SIZE_MAX - info->nameLen - 1u) {
    entries->overflow = true;
    return false;
  }
  entries->count++;
  entries->chars += info->nameLen + 1u;
  return true;
}

static
bool
ak_package_copy_entry(const AkZipEntryInfo * __restrict info,
                      void                 * __restrict userdata) {
  AkPackageEntries *entries;
  AkPackageEntry   *entry;

  entries = userdata;
  if (entries->next >= entries->count)
    return false;

  entry                   = &entries->items[entries->next++];
  entry->name             = entries->cursor;
  entry->index            = info->index;
  entry->uncompressedSize = info->uncompressedSize;
  entry->method           = info->method;
  entry->flags            = info->flags;
  memcpy(entries->cursor, info->name, info->nameLen);
  entries->cursor[info->nameLen] = '\0';
  entries->cursor += info->nameLen + 1u;
  return true;
}

static
void
ak_package_entries_release(AkPackageEntries * __restrict entries) {
  if (!entries)
    return;

  free(entries->storage);
  free(entries->items);
  memset(entries, 0, sizeof(*entries));
}

static
bool
ak_package_entries_load(AkZipArchive     * __restrict archive,
                        AkPackageEntries * __restrict entries) {
  memset(entries, 0, sizeof(*entries));
  if (ak_zip_archive_visit_entries(archive,
                                   ak_package_measure_entry,
                                   entries) != AK_OK
      || entries->overflow
      || entries->count == 0u
      || entries->chars == 0u) {
    return false;
  }

  if (entries->count > (size_t)-1 / sizeof(*entries->items))
    return false;

  entries->items   = calloc(entries->count, sizeof(*entries->items));
  entries->storage = malloc(entries->chars);
  if (!entries->items || !entries->storage) {
    ak_package_entries_release(entries);
    return false;
  }

  entries->cursor = entries->storage;
  if (ak_zip_archive_visit_entries(archive,
                                   ak_package_copy_entry,
                                   entries) != AK_OK
      || entries->next != entries->count) {
    ak_package_entries_release(entries);
    return false;
  }

  return true;
}

static
bool
ak_package_path_normalize(const char * __restrict input,
                          char       * __restrict output,
                          size_t                  capacity,
                          unsigned   * __restrict depth,
                          bool       * __restrict directory) {
  size_t out;
  size_t i;
  unsigned segments;
  bool trailingSlash;

  if (!input || !*input || !output || capacity == 0u)
    return false;
  if (input[0] == '/' || input[0] == '\\')
    return false;

  out           = 0u;
  i             = 0u;
  segments      = 0u;
  trailingSlash = false;
  while (input[i]) {
    size_t segmentStart;
    size_t segmentLen;

    while (input[i] == '/' || input[i] == '\\') {
      trailingSlash = true;
      i++;
    }
    if (!input[i])
      break;

    trailingSlash = false;
    segmentStart  = i;
    while (input[i] && input[i] != '/' && input[i] != '\\') {
      unsigned char c;

      c = (unsigned char)input[i];
      if (c < 0x20u || c == ':' || c == 0x7fu)
        return false;
      i++;
    }
    segmentLen = i - segmentStart;
    if (segmentLen == 1u && input[segmentStart] == '.')
      continue;
    if (segmentLen == 2u
        && input[segmentStart] == '.'
        && input[segmentStart + 1u] == '.') {
      return false;
    }
    if (segmentLen == 0u)
      continue;

    if (out > 0u) {
      if (out + 1u >= capacity)
        return false;
      output[out++] = '/';
    }
    if (segmentLen >= capacity - out)
      return false;

    memcpy(output + out, input + segmentStart, segmentLen);
    out += segmentLen;
    segments++;
  }

  if (out == 0u || segments == 0u)
    return false;

  output[out] = '\0';
  if (depth)
    *depth = segments - 1u;
  if (directory)
    *directory = trailingSlash;
  return true;
}

static
bool
ak_package_ascii_eq_ci(const char *a, const char *b) {
  unsigned char ca;
  unsigned char cb;

  while (*a && *b) {
    ca = (unsigned char)*a++;
    cb = (unsigned char)*b++;
    if (ca >= 'A' && ca <= 'Z')
      ca = (unsigned char)(ca + ('a' - 'A'));
    if (cb >= 'A' && cb <= 'Z')
      cb = (unsigned char)(cb + ('a' - 'A'));
    if (ca != cb)
      return false;
  }

  return *a == '\0' && *b == '\0';
}

static
bool
ak_package_supported_root(const char * __restrict path) {
  const char *base;
  const char *ext;

  base = strrchr(path, '/');
  base = base ? base + 1 : path;
  ext  = strrchr(base, '.');
  if (!ext || ext == base || ext[1] == '\0')
    return false;
  ext++;

  return ak_package_ascii_eq_ci(ext, "gltf")
         || ak_package_ascii_eq_ci(ext, "glb")
         || ak_package_ascii_eq_ci(ext, "dae")
         || ak_package_ascii_eq_ci(ext, "obj")
         || ak_package_ascii_eq_ci(ext, "stl")
         || ak_package_ascii_eq_ci(ext, "ply")
         || ak_package_ascii_eq_ci(ext, "3mf")
         || ak_package_ascii_eq_ci(ext, "zae")
         || ak_package_ascii_eq_ci(ext, "kmz");
}

static
bool
ak_package_is_dae(const char * __restrict path) {
  const char *ext;

  ext = strrchr(path, '.');
  return ext && ak_package_ascii_eq_ci(ext + 1, "dae");
}

static
const AkPackageEntry*
ak_package_pick_root(const AkPackageEntries * __restrict entries) {
  const AkPackageEntry *best;
  unsigned              bestDepth;
  size_t                i;

  best      = NULL;
  bestDepth = UINT_MAX;
  for (i = 0u; i < entries->count; i++) {
    char     normalized[PATH_MAX];
    unsigned depth;
    bool     directory;

    if (!ak_package_path_normalize(entries->items[i].name,
                                   normalized,
                                   sizeof(normalized),
                                   &depth,
                                   &directory)
        || directory
        || !ak_package_supported_root(normalized)) {
      continue;
    }

    if (!best || depth < bestDepth) {
      best      = &entries->items[i];
      bestDepth = depth;
    }
  }

  return best;
}

static
char*
ak_package_temp_dir(void) {
#if defined(AK_WINAPI)
  char     base[MAX_PATH];
  char    *path;
  DWORD    baseLen;
  DWORD    pid;
  ULONGLONG tick;
  unsigned attempt;

  baseLen = GetTempPathA((DWORD)sizeof(base), base);
  if (baseLen == 0u || baseLen >= sizeof(base))
    return NULL;

  path = malloc((size_t)baseLen + 96u);
  if (!path)
    return NULL;

  pid  = GetCurrentProcessId();
  tick = GetTickCount64();
  for (attempt = 0u; attempt < 128u; attempt++) {
    snprintf(path,
             (size_t)baseLen + 96u,
             "%sassetkit-zip-%lu-%llu-%u",
             base,
             (unsigned long)pid,
             (unsigned long long)tick,
             attempt);
    if (CreateDirectoryA(path, NULL))
      return path;
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      break;
  }

  free(path);
  return NULL;
#else
  const char *base;
  char       *path;
  size_t      baseLen;
  bool        slash;

  base = getenv("TMPDIR");
  if (!base || !*base)
    base = "/tmp";
  baseLen = strlen(base);
  slash   = baseLen > 0u && base[baseLen - 1u] != '/';
  if (baseLen > (size_t)-1 - 32u)
    return NULL;

  path = malloc(baseLen + (slash ? 1u : 0u) + 24u);
  if (!path)
    return NULL;

  memcpy(path, base, baseLen);
  if (slash)
    path[baseLen++] = '/';
  memcpy(path + baseLen, "assetkit-zip-XXXXXX", 21u);
  if (!mkdtemp(path)) {
    free(path);
    return NULL;
  }
  return path;
#endif
}

static
int
ak_package_rmdir(const char * __restrict path) {
#if defined(AK_WINAPI)
  return _rmdir(path);
#else
  return rmdir(path);
#endif
}

static
bool
ak_package_write_file(const char * __restrict path,
                      const void * __restrict data,
                      size_t                  size) {
  FILE *file;
  bool  ok;

  file = fopen(path, "wb");
  if (!file)
    return false;

  ok = size == 0u || fwrite(data, 1u, size, file) == size;
  if (fclose(file) != 0)
    ok = false;
  if (!ok)
    remove(path);
  return ok;
}

static
bool
ak_package_extract_entries(AkZipArchive          * __restrict archive,
                           const AkPackageEntries * __restrict entries,
                           const char             * __restrict tempDir) {
  AkZipDecompressor *decompressor;
  size_t             i;
  bool               ok;

  decompressor = ak_zip_decompressor_new();
  if (!decompressor)
    return false;

  ok = true;
  for (i = 0u; i < entries->count; i++) {
    const AkPackageEntry *entry;
    char                  normalized[PATH_MAX];
    char                 *path;
    void                 *data;
    size_t                size;
    bool                  directory;

    entry = &entries->items[i];
    if (!ak_package_path_normalize(entry->name,
                                   normalized,
                                   sizeof(normalized),
                                   NULL,
                                   &directory)
        || directory
        || (entry->flags & 1u) != 0u
        || (entry->method != AK_PACKAGE_ZIP_METHOD_STORED
            && entry->method != AK_PACKAGE_ZIP_METHOD_DEFLATED)) {
      continue;
    }

    path = io_path_join_dup(tempDir, normalized);
    if (!path) {
      ok = false;
      break;
    }
    if (!io_path_mkdir_parent_dirs(path, true)) {
      free(path);
      ok = false;
      break;
    }

    if ((uint64_t)entry->uncompressedSize + 1u > SIZE_MAX) {
      free(path);
      ok = false;
      break;
    }
    data = malloc((size_t)entry->uncompressedSize + 1u);
    if (!data) {
      free(path);
      ok = false;
      break;
    }
    size = 0u;
    if (ak_zip_archive_extract_index_to(archive,
                                        entry->index,
                                        decompressor,
                                        data,
                                        (size_t)entry->uncompressedSize + 1u,
                                        &size) != AK_OK
        || size != entry->uncompressedSize
        || !ak_package_write_file(path, data, size)) {
      free(data);
      free(path);
      ok = false;
      break;
    }

    free(data);
    free(path);
  }

  ak_zip_decompressor_free(decompressor);
  return ok;
}

static
void
ak_package_remove_entries(const AkPackageEntries * __restrict entries,
                          const char             * __restrict tempDir) {
  size_t i;

  for (i = entries->count; i > 0u; i--) {
    char  normalized[PATH_MAX];
    char *path;
    char *slash;
    bool  directory;

    if (!ak_package_path_normalize(entries->items[i - 1u].name,
                                   normalized,
                                   sizeof(normalized),
                                   NULL,
                                   &directory)
        || directory) {
      continue;
    }

    path = io_path_join_dup(tempDir, normalized);
    if (!path)
      continue;
    remove(path);
    slash = strrchr(path, '/');
    while (slash && slash > path + strlen(tempDir)) {
      *slash = '\0';
      ak_package_rmdir(path);
      slash = strrchr(path, '/');
    }
    free(path);
  }

  ak_package_rmdir(tempDir);
}

static
const char*
ak_package_image_mime(const char *path) {
  const char *ext;

  ext = strrchr(path, '.');
  if (!ext)
    return NULL;
  ext++;
  if (ak_package_ascii_eq_ci(ext, "png"))
    return "image/png";
  if (ak_package_ascii_eq_ci(ext, "jpg")
      || ak_package_ascii_eq_ci(ext, "jpeg")) {
    return "image/jpeg";
  }
  if (ak_package_ascii_eq_ci(ext, "webp"))
    return "image/webp";
  if (ak_package_ascii_eq_ci(ext, "dds"))
    return "image/vnd-ms.dds";
  if (ak_package_ascii_eq_ci(ext, "ktx2"))
    return "image/ktx2";
  return NULL;
}

static
void
ak_package_attach_images(AkDoc * __restrict doc) {
  AkHeap  *heap;
  AkImage *image;

  if (!doc)
    return;

  heap = ak_heap_getheap(doc);
  for (image = doc->lib.images.first; image; image = image->next) {
    AkImageSource *source;
    AkBuffer      *buffer;
    char          *path;
    const char    *mime;

    source = ak_imageSource(image);
    if (!source || source->type != AK_IMAGE_SOURCE_URI || !source->uri)
      continue;

    path = ak_getFileFrom(doc, source->uri);
    if (!path)
      continue;

    buffer = ak_heap_calloc(heap, source, sizeof(*buffer));
    if (!buffer) {
      ak_free(path);
      continue;
    }

    if (ak_readfile(path,
                    buffer,
                    &buffer->data,
                    &buffer->length) != AK_OK) {
      ak_free(buffer);
      ak_free(path);
      continue;
    }

    buffer->name = ak_heap_strdup(heap, buffer, source->uri);
    mime         = ak_package_image_mime(source->uri);
    if (!source->mimeType && mime)
      source->mimeType = mime;
    source->buffer = buffer;
    source->type   = AK_IMAGE_SOURCE_BUFFER;
    ak_free(path);
  }
}

static
void
ak_package_restore_document_path(AkDoc      * __restrict doc,
                                 const char * __restrict archivePath) {
  AkHeap *heap;
  char   *name;
  char   *dir;

  if (!doc || !doc->inf || !archivePath)
    return;

  heap = ak_heap_getheap(doc);
  name = ak_heap_strdup(heap, doc->inf, archivePath);
  dir  = ak_path_dir(heap, doc->inf, archivePath);
  if (name) {
    if (doc->inf->name)
      ak_free((void *)doc->inf->name);
    doc->inf->name = name;
  }
  if (dir) {
    if (doc->inf->dir)
      ak_free((void *)doc->inf->dir);
    doc->inf->dir    = dir;
    doc->inf->dirlen = strlen(dir);
  }
}

AK_HIDE
AkResult
ak_zip_package_doc(AkDoc      ** __restrict dest,
                   const char  * __restrict filepath) {
  AkPackageEntries     entries;
  AkZipArchive        *archive;
  const AkPackageEntry *root;
  char                 normalizedRoot[PATH_MAX];
  char                *tempDir;
  char                *rootPath;
  const char          *previousScope;
  AkDoc               *doc;
  AkResult             result;

  if (!dest || !filepath)
    return AK_ERR;

  *dest         = NULL;
  archive       = NULL;
  tempDir       = NULL;
  rootPath      = NULL;
  previousScope = NULL;
  doc           = NULL;
  result        = AK_ERR;
  memset(&entries, 0, sizeof(entries));

  result = ak_zip_open(filepath, &archive);
  if (result != AK_OK)
    goto cleanup;
  if (!ak_package_entries_load(archive, &entries)) {
    result = AK_EBADF;
    goto cleanup;
  }

  root = ak_package_pick_root(&entries);
  if (!root
      || !ak_package_path_normalize(root->name,
                                    normalizedRoot,
                                    sizeof(normalizedRoot),
                                    NULL,
                                    NULL)) {
    result = AK_EBADF;
    goto cleanup;
  }

  if (ak_package_is_dae(normalizedRoot)) {
    result = dae_archive_doc_archive(dest,
                                     filepath,
                                     archive,
                                     root->name);
    goto cleanup;
  }

  tempDir = ak_package_temp_dir();
  if (!tempDir) {
    result = AK_ERR;
    goto cleanup;
  }
  if (!ak_package_extract_entries(archive, &entries, tempDir)) {
    result = AK_EBADF;
    goto cleanup;
  }

  rootPath = io_path_join_dup(tempDir, normalizedRoot);
  if (!rootPath) {
    result = AK_ERR;
    goto cleanup;
  }

  previousScope = ak_file_scope_set(tempDir);
  result        = ak_load(&doc, rootPath, AK_FILE_TYPE_AUTO);
  if (result == AK_OK && doc) {
    ak_package_attach_images(doc);
    ak_package_restore_document_path(doc, filepath);
    *dest = doc;
    doc   = NULL;
  } else if (result == AK_OK) {
    result = AK_ERR;
  }
  ak_file_scope_set(previousScope);
  previousScope = NULL;

cleanup:
  if (previousScope)
    ak_file_scope_set(previousScope);
  if (doc)
    ak_free(doc);
  if (tempDir)
    ak_package_remove_entries(&entries, tempDir);
  free(rootPath);
  free(tempDir);
  ak_package_entries_release(&entries);
  ak_zip_close(archive);
  return result;
}
