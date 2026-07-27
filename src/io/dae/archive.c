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

#include "dae.h"
#include "../common/zip.h"
#include "../../image/export.h"
#include "../../xml.h"

#include <xml/xml.h>
#include <xml/util.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct DAEArchiveNames {
  char   **items;
  char    *storage;
  size_t   count;
  size_t   chars;
  size_t   next;
  char    *cursor;
} DAEArchiveNames;

typedef struct DAEArchiveImageAlloc {
  AkHeap   *heap;
  AkBuffer *buffer;
} DAEArchiveImageAlloc;

static
bool
dae_archive_measure_name(const AkZipEntryInfo * __restrict info,
                         void                 * __restrict userdata) {
  DAEArchiveNames *names;

  names = userdata;
  names->count++;
  names->chars += info->nameLen + 1u;
  return true;
}

static
bool
dae_archive_copy_name(const AkZipEntryInfo * __restrict info,
                      void                 * __restrict userdata) {
  DAEArchiveNames *names;

  names = userdata;
  if (names->next >= names->count)
    return false;

  names->items[names->next++] = names->cursor;
  memcpy(names->cursor, info->name, info->nameLen);
  names->cursor[info->nameLen] = '\0';
  names->cursor += info->nameLen + 1u;
  return true;
}

static
void
dae_archive_names_release(DAEArchiveNames * __restrict names) {
  if (!names)
    return;

  free(names->storage);
  free(names->items);
  memset(names, 0, sizeof(*names));
}

static
bool
dae_archive_names_load(AkZipArchive   * __restrict archive,
                       DAEArchiveNames * __restrict names) {
  memset(names, 0, sizeof(*names));
  if (ak_zip_archive_visit_entries(archive,
                                   dae_archive_measure_name,
                                   names) != AK_OK
      || names->count == 0u
      || names->chars == 0u) {
    return false;
  }

  names->items   = malloc(sizeof(*names->items) * names->count);
  names->storage = malloc(names->chars);
  if (!names->items || !names->storage) {
    dae_archive_names_release(names);
    return false;
  }

  names->cursor = names->storage;
  if (ak_zip_archive_visit_entries(archive,
                                   dae_archive_copy_name,
                                   names) != AK_OK
      || names->next != names->count) {
    dae_archive_names_release(names);
    return false;
  }

  return true;
}

static
int
dae_archive_hex_value(unsigned char c) {
  if (c >= '0' && c <= '9')
    return (int)(c - '0');
  if (c >= 'a' && c <= 'f')
    return (int)(c - 'a') + 10;
  if (c >= 'A' && c <= 'F')
    return (int)(c - 'A') + 10;
  return -1;
}

static
bool
dae_archive_path_normalize(const char * __restrict input,
                           char       * __restrict output,
                           size_t                  capacity) {
  size_t segmentStarts[128];
  size_t segmentCount;
  size_t out;
  size_t i;

  if (!input || !output || capacity == 0u)
    return false;

  while (isspace((unsigned char)*input))
    input++;
  if ((input[0] == 'f' || input[0] == 'F')
      && (input[1] == 'i' || input[1] == 'I')
      && (input[2] == 'l' || input[2] == 'L')
      && (input[3] == 'e' || input[3] == 'E')
      && input[4] == ':') {
    input += 5;
  } else {
    const char *scan;

    scan = input;
    while (*scan && *scan != '/' && *scan != '\\'
           && *scan != '#' && *scan != '?') {
      if (*scan == ':')
        return false;
      scan++;
    }
  }

  while (*input == '/' || *input == '\\')
    input++;

  segmentCount = 0u;
  out          = 0u;
  i            = 0u;
  while (input[i] && input[i] != '#' && input[i] != '?') {
    char   segment[1024];
    size_t segmentLen;

    while (input[i] == '/' || input[i] == '\\')
      i++;
    if (!input[i] || input[i] == '#' || input[i] == '?')
      break;

    segmentLen = 0u;
    while (input[i] && input[i] != '/' && input[i] != '\\'
           && input[i] != '#' && input[i] != '?') {
      unsigned char c;

      c = (unsigned char)input[i++];
      if (c == '%' && input[i] && input[i + 1u]) {
        int hi, lo;

        hi = dae_archive_hex_value((unsigned char)input[i]);
        lo = dae_archive_hex_value((unsigned char)input[i + 1u]);
        if (hi >= 0 && lo >= 0) {
          c = (unsigned char)((hi << 4) | lo);
          i += 2u;
        }
      }

      if (c == '\0' || segmentLen + 1u >= sizeof(segment))
        return false;
      segment[segmentLen++] = (char)c;
    }
    while (segmentLen > 0u
           && isspace((unsigned char)segment[segmentLen - 1u])) {
      segmentLen--;
    }

    if (segmentLen == 0u
        || (segmentLen == 1u && segment[0] == '.')) {
      continue;
    }
    if (segmentLen == 2u && segment[0] == '.' && segment[1] == '.') {
      if (segmentCount == 0u)
        return false;
      out = segmentStarts[--segmentCount];
      if (out > 0u)
        out--;
      continue;
    }

    if (segmentCount >= sizeof(segmentStarts) / sizeof(segmentStarts[0]))
      return false;
    if (out > 0u) {
      if (out + 1u >= capacity)
        return false;
      output[out++] = '/';
    }
    if (segmentLen >= capacity - out)
      return false;
    segmentStarts[segmentCount++] = out;
    memcpy(output + out, segment, segmentLen);
    out += segmentLen;
  }

  output[out] = '\0';
  return out > 0u;
}

static
bool
dae_archive_ascii_eq_ci(const char *a, const char *b) {
  unsigned char ca, cb;

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
dae_archive_ends_with_ci(const char *value, const char *suffix) {
  size_t valueLen, suffixLen;

  valueLen  = strlen(value);
  suffixLen = strlen(suffix);
  if (suffixLen > valueLen)
    return false;
  return dae_archive_ascii_eq_ci(value + valueLen - suffixLen, suffix);
}

static
const char*
dae_archive_find(const DAEArchiveNames * __restrict names,
                 const char            * __restrict wanted,
                 bool                               uniqueBasenameFallback) {
  const char *exact;
  const char *folded;
  const char *base;
  const char *baseHit;
  char        normalized[4096];
  char        candidate[4096];
  size_t      foldedCount;
  size_t      baseCount;
  size_t      i;

  if (!dae_archive_path_normalize(wanted,
                                  normalized,
                                  sizeof(normalized))) {
    return NULL;
  }

  exact       = NULL;
  folded      = NULL;
  foldedCount = 0u;
  baseHit     = NULL;
  baseCount   = 0u;
  base        = strrchr(normalized, '/');
  base        = base ? base + 1 : normalized;

  for (i = 0u; i < names->count; i++) {
    const char *candidateBase;

    if (!dae_archive_path_normalize(names->items[i],
                                    candidate,
                                    sizeof(candidate))) {
      continue;
    }
    if (strcmp(candidate, normalized) == 0)
      exact = names->items[i];
    if (dae_archive_ascii_eq_ci(candidate, normalized)) {
      folded = names->items[i];
      foldedCount++;
    }

    if (!uniqueBasenameFallback)
      continue;
    candidateBase = strrchr(candidate, '/');
    candidateBase = candidateBase ? candidateBase + 1 : candidate;
    if (dae_archive_ascii_eq_ci(candidateBase, base)) {
      baseHit = names->items[i];
      baseCount++;
    }
  }

  if (exact)
    return exact;
  if (foldedCount == 1u)
    return folded;
  if (uniqueBasenameFallback && baseCount == 1u)
    return baseHit;
  return NULL;
}

static
char*
dae_archive_manifest_root(AkZipArchive          * __restrict archive,
                          const DAEArchiveNames * __restrict names) {
  const char *manifestName;
  const xml_t *text;
  xml_doc_t  *xdoc;
  void       *data;
  char       *rootPath;
  char       *start;
  char       *end;
  size_t      size;
  size_t      length;

  manifestName = dae_archive_find(names, "manifest.xml", true);
  if (!manifestName)
    return NULL;

  data = NULL;
  size = 0u;
  if (ak_zip_archive_extract_file(archive,
                                  manifestName,
                                  &data,
                                  &size) != AK_OK) {
    return NULL;
  }

  xdoc = xml_parse(data, XML_PREFIXES | XML_READONLY);
  if (!xdoc || !xdoc->root || !(text = xmls(xdoc->root))) {
    if (xdoc)
      xml_free(xdoc);
    free(data);
    return NULL;
  }

  length = 0u;
  for (; text; text = xmls_next(text))
    length += text->valsize;
  rootPath = malloc(length + 1u);
  if (!rootPath) {
    xml_free(xdoc);
    free(data);
    return NULL;
  }

  start = rootPath;
  for (text = xmls(xdoc->root); text; text = xmls_next(text)) {
    memcpy(start, text->val, text->valsize);
    start += text->valsize;
  }
  *start = '\0';

  start = rootPath;
  while (isspace((unsigned char)*start))
    start++;
  end = rootPath + length;
  while (end > start && isspace((unsigned char)end[-1]))
    end--;
  *end = '\0';
  if (start != rootPath)
    memmove(rootPath, start, (size_t)(end - start) + 1u);

  xml_free(xdoc);
  free(data);
  return rootPath;
}

static
const char*
dae_archive_pick_root(AkZipArchive          * __restrict archive,
                      const DAEArchiveNames * __restrict names) {
  static const char *preferred[] = {
    "doc.dae",
    "scene.dae",
    "model.dae",
    "models/model.dae"
  };
  const char *root;
  const char *bestTop;
  const char *bestAny;
  char       *manifestRoot;
  char        normalized[4096];
  size_t      i;

  manifestRoot = dae_archive_manifest_root(archive, names);
  if (manifestRoot) {
    root = dae_archive_find(names, manifestRoot, true);
    free(manifestRoot);
    if (root && dae_archive_ends_with_ci(root, ".dae"))
      return root;
  }

  for (i = 0u; i < sizeof(preferred) / sizeof(preferred[0]); i++) {
    root = dae_archive_find(names, preferred[i], false);
    if (root)
      return root;
  }

  bestTop = NULL;
  bestAny = NULL;
  for (i = 0u; i < names->count; i++) {
    if (!dae_archive_path_normalize(names->items[i],
                                    normalized,
                                    sizeof(normalized))
        || !dae_archive_ends_with_ci(normalized, ".dae")) {
      continue;
    }

    if (!bestAny || strcmp(names->items[i], bestAny) < 0)
      bestAny = names->items[i];
    if (!strchr(normalized, '/')
        && (!bestTop || strcmp(names->items[i], bestTop) < 0)) {
      bestTop = names->items[i];
    }
  }

  return bestTop ? bestTop : bestAny;
}

static
void*
dae_archive_image_alloc(void * __restrict userdata, size_t size) {
  DAEArchiveImageAlloc *ctx;

  ctx = userdata;
  return ak_heap_alloc(ctx->heap, ctx->buffer, size);
}

static
void
dae_archive_image_free(void * __restrict userdata,
                       void * __restrict data) {
  (void)userdata;
  ak_free(data);
}

static
const char*
dae_archive_image_mime(const char *path) {
  if (dae_archive_ends_with_ci(path, ".png"))
    return "image/png";
  if (dae_archive_ends_with_ci(path, ".jpg")
      || dae_archive_ends_with_ci(path, ".jpeg")) {
    return "image/jpeg";
  }
  if (dae_archive_ends_with_ci(path, ".webp"))
    return "image/webp";
  if (dae_archive_ends_with_ci(path, ".dds"))
    return "image/vnd-ms.dds";
  if (dae_archive_ends_with_ci(path, ".ktx2"))
    return "image/ktx2";
  return NULL;
}

static
bool
dae_archive_uri_is_relative(const char *uri) {
  const char *scan;

  if (!uri)
    return false;
  while (isspace((unsigned char)*uri))
    uri++;
  if (*uri == '/' || *uri == '\\')
    return false;

  scan = uri;
  while (*scan && *scan != '/' && *scan != '\\'
         && *scan != '#' && *scan != '?') {
    if (*scan == ':')
      return false;
    scan++;
  }
  return true;
}

static
void
dae_archive_attach_images(AkDoc                 * __restrict doc,
                          AkZipArchive          * __restrict archive,
                          const DAEArchiveNames * __restrict names,
                          const char            * __restrict daeRoot) {
  AkHeap  *heap;
  AkImage *image;
  char     rootNorm[4096];
  char     rootDir[4096];
  char     uriNorm[4096];
  char     candidate[4096];
  char    *slash;
  size_t   rootDirLen;

  if (!doc || !archive || !names || !daeRoot
      || !dae_archive_path_normalize(daeRoot,
                                     rootNorm,
                                     sizeof(rootNorm))) {
    return;
  }

  memcpy(rootDir, rootNorm, strlen(rootNorm) + 1u);
  slash = strrchr(rootDir, '/');
  if (slash)
    *slash = '\0';
  else
    rootDir[0] = '\0';
  rootDirLen = strlen(rootDir);
  heap       = ak_heap_getheap(doc);

  for (image = doc->lib.images.first; image; image = image->next) {
    AkImageSource        *source;
    AkBuffer             *buffer;
    DAEArchiveImageAlloc  allocCtx;
    const char           *entry;
    const char           *mime;
    void                 *data;
    size_t                size;

    source = ak_imageSource(image);
    if (!source || source->type != AK_IMAGE_SOURCE_URI || !source->uri)
      continue;

    entry = NULL;
    if (rootDirLen > 0u
        && dae_archive_uri_is_relative(source->uri)
        && rootDirLen + 1u + strlen(source->uri) < sizeof(candidate)) {
      memcpy(candidate, rootDir, rootDirLen);
      candidate[rootDirLen] = '/';
      memcpy(candidate + rootDirLen + 1u,
             source->uri,
             strlen(source->uri) + 1u);
      entry = dae_archive_find(names, candidate, false);
    }
    if (!entry
        && dae_archive_path_normalize(source->uri,
                                      uriNorm,
                                      sizeof(uriNorm))) {
      entry = dae_archive_find(names, uriNorm, true);
    }
    if (!entry)
      continue;

    buffer = ak_heap_calloc(heap, source, sizeof(*buffer));
    if (!buffer)
      continue;
    allocCtx.heap   = heap;
    allocCtx.buffer = buffer;
    data            = NULL;
    size            = 0u;
    if (ak_zip_archive_extract_file_alloc(archive,
                                          entry,
                                          dae_archive_image_alloc,
                                          dae_archive_image_free,
                                          &allocCtx,
                                          &data,
                                          &size) != AK_OK) {
      ak_free(buffer);
      continue;
    }

    buffer->name   = ak_heap_strdup(heap, buffer, entry);
    buffer->data   = data;
    buffer->length = size;
    mime           = dae_archive_image_mime(entry);
    if (!source->mimeType && mime)
      source->mimeType = mime;
    source->buffer = buffer;
    source->type   = AK_IMAGE_SOURCE_BUFFER;
  }
}

static
AkResult
dae_archive_doc_open(AkDoc          ** __restrict dest,
                     const char      * __restrict filepath,
                     AkZipArchive    * __restrict archive,
                     const char      * __restrict entryName) {
  DAEArchiveNames names;
  const char     *daeRoot;
  void           *daeData;
  size_t          daeSize;
  AkDoc          *doc;
  AkResult        result;

  if (!dest || !filepath || !archive)
    return AK_ERR;

  *dest   = NULL;
  daeData = NULL;
  daeSize = 0u;
  doc     = NULL;
  memset(&names, 0, sizeof(names));

  if (!dae_archive_names_load(archive, &names)) {
    result = AK_EBADF;
    goto cleanup;
  }

  daeRoot = entryName
            ? dae_archive_find(&names, entryName, false)
            : dae_archive_pick_root(archive, &names);
  if (!daeRoot) {
    result = AK_EBADF;
    goto cleanup;
  }
  result = ak_zip_archive_extract_file(archive,
                                       daeRoot,
                                       &daeData,
                                       &daeSize);
  if (result != AK_OK)
    goto cleanup;

  result  = dae_doc_memory(&doc, filepath, daeData, daeSize);
  daeData = NULL;
  if (result != AK_OK || !doc)
    goto cleanup;

  dae_archive_attach_images(doc, archive, &names, daeRoot);
  *dest = doc;

cleanup:
  if (daeData)
    free(daeData);
  dae_archive_names_release(&names);
  return result;
}

AK_HIDE
AkResult
dae_archive_doc(AkDoc      ** __restrict dest,
                const char  * __restrict filepath) {
  AkZipArchive *archive;
  AkResult      result;

  if (!dest || !filepath)
    return AK_ERR;

  *dest   = NULL;
  archive = NULL;
  result  = ak_zip_open(filepath, &archive);
  if (result == AK_OK)
    result = dae_archive_doc_open(dest, filepath, archive, NULL);
  ak_zip_close(archive);
  return result;
}

AK_HIDE
AkResult
dae_archive_doc_archive(AkDoc          ** __restrict dest,
                        const char      * __restrict filepath,
                        AkZipArchive    * __restrict archive,
                        const char      * __restrict entryName) {
  if (!entryName || !archive)
    return AK_ERR;
  return dae_archive_doc_open(dest, filepath, archive, entryName);
}
