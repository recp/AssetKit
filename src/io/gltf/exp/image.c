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

#include "image.h"
#include "../strpool.h"
#include "../../common/uri.h"
#include "../../../image/export.h"

#include <ak/path.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 260
#endif

AkImageSource*
gltf_image_source(AkImage * __restrict image) {
  if (!image)
    return NULL;

  if (image->source)
    return image->source;

  return image->image ? image->image->source : NULL;
}

bool
gltf_image_source_supported(AkImageSource * __restrict source) {
  if (!source)
    return false;

  switch (source->type) {
    case AK_IMAGE_SOURCE_URI:
      return source->uri != NULL;
    case AK_IMAGE_SOURCE_BUFFER:
      return source->buffer
             && source->buffer->data
             && source->buffer->length > 0
             && source->mimeType != NULL;
    default:
      break;
  }

  return false;
}

bool
gltf_image_uri_is_data(const char * __restrict uri) {
  return io_uri_has_prefix(uri, _s_gltf_b64d, _s_gltf_b64d_len);
}

bool
gltf_image_uri_has_scheme(const char * __restrict uri) {
  return io_uri_has_scheme(uri);
}

bool
gltf_image_uri_is_file_scheme(const char * __restrict uri) {
  return io_uri_has_prefix(uri, _s_gltf_file_uri, _s_gltf_file_uri_len);
}

bool
gltf_image_path_is_abs(const char * __restrict path) {
  return io_path_is_abs_drive_colon(path);
}

static
const char*
gltf_image_uri_file_path(const char * __restrict uri) {
  const char *path;

  if (!uri)
    return NULL;

  if (gltf_image_uri_is_file_scheme(uri)) {
    path = uri + 7u;
#ifdef _WIN32
    if (path[0] == '/'
        && (((path[1] >= 'A' && path[1] <= 'Z')
             || (path[1] >= 'a' && path[1] <= 'z'))
            && path[2] == ':'))
      path++;
#endif
    return path;
  }

  return gltf_image_path_is_abs(uri) ? uri : NULL;
}

static
int
gltf_image_uri_hex(unsigned char c) {
  if (c >= '0' && c <= '9')
    return (int)(c - '0');
  if (c >= 'A' && c <= 'F')
    return (int)(c - 'A') + 10;
  if (c >= 'a' && c <= 'f')
    return (int)(c - 'a') + 10;
  return -1;
}

static
bool
gltf_image_uri_decode_path(const char * __restrict uri,
                           char       * __restrict dst,
                           size_t                  dstCap) {
  size_t i;
  size_t j;

  if (!uri || !dst || dstCap == 0)
    return false;

  i = 0;
  j = 0;
  while (uri[i]) {
    unsigned char c;

    if (j + 1u >= dstCap)
      return false;

    c = (unsigned char)uri[i];
    if (c == '%' && uri[i + 1] && uri[i + 2]) {
      int hi;
      int lo;

      hi = gltf_image_uri_hex((unsigned char)uri[i + 1]);
      lo = gltf_image_uri_hex((unsigned char)uri[i + 2]);
      if (hi >= 0 && lo >= 0) {
        dst[j++] = (char)((hi << 4) | lo);
        i += 3u;
        continue;
      }
    }

    dst[j++] = (char)c;
    i++;
  }

  dst[j] = '\0';
  return true;
}

const char*
gltf_image_source_path(GLTFExpState  * __restrict st,
                       AkImageSource * __restrict source,
                       char          * __restrict pathbuf) {
  const char *filePath;
  char        relbuf[PATH_MAX];

  if (!source || !source->uri || gltf_image_uri_is_data(source->uri))
    return NULL;

  if (source->resolvedPath)
    return source->resolvedPath;

  filePath = gltf_image_uri_file_path(source->uri);
  if (filePath) {
    if (strchr(filePath, '%')) {
      if (!gltf_image_uri_decode_path(filePath, pathbuf, PATH_MAX))
        return NULL;
      return pathbuf;
    }
    return filePath;
  }

  if (gltf_image_uri_has_scheme(source->uri))
    return NULL;

  if (st->doc && st->doc->inf && st->doc->inf->dir) {
    const char *uriPath;

    uriPath = source->uri;
    if (strchr(uriPath, '%')) {
      if (!gltf_image_uri_decode_path(uriPath, relbuf, sizeof(relbuf)))
        return NULL;
      uriPath = relbuf;
    }

    return ak_fullpath(st->doc, uriPath, pathbuf);
  }

  return NULL;
}

static
bool
gltf_image_cstr_eq_token(const char * __restrict val,
                         const char * __restrict token,
                         size_t                  tokenLen) {
  return val && strlen(val) == tokenLen && memcmp(val, token, tokenLen) == 0;
}

bool
gltf_image_mime_supported(const char * __restrict mimeType) {
  return gltf_image_cstr_eq_token(mimeType,
                                  _s_gltf_mime_image_png,
                                  _s_gltf_mime_image_png_len)
         || gltf_image_cstr_eq_token(mimeType,
                                     _s_gltf_mime_image_jpeg,
                                     _s_gltf_mime_image_jpeg_len)
         || gltf_image_cstr_eq_token(mimeType,
                                     _s_gltf_mime_image_webp,
                                     _s_gltf_mime_image_webp_len)
         || gltf_image_cstr_eq_token(mimeType,
                                     _s_gltf_mime_image_ktx2,
                                     _s_gltf_mime_image_ktx2_len);
}

static
const char*
gltf_image_data_uri_mime(const char * __restrict uri) {
  const char *start;
  const char *end;
  size_t      len;

  if (!gltf_image_uri_is_data(uri))
    return NULL;

  start = uri + _s_gltf_b64d_len;
  end   = strchr(start, ';');
  if (!end || end <= start)
    return NULL;

  len = (size_t)(end - start);
  if (len == _s_gltf_mime_image_png_len
      && memcmp(start, _s_gltf_mime_image_png, len) == 0)
    return _s_gltf_mime_image_png;
  if (len == _s_gltf_mime_image_jpeg_len
      && memcmp(start, _s_gltf_mime_image_jpeg, len) == 0)
    return _s_gltf_mime_image_jpeg;
  if (len == _s_gltf_mime_image_webp_len
      && memcmp(start, _s_gltf_mime_image_webp, len) == 0)
    return _s_gltf_mime_image_webp;
  if (len == _s_gltf_mime_image_ktx2_len
      && memcmp(start, _s_gltf_mime_image_ktx2, len) == 0)
    return _s_gltf_mime_image_ktx2;

  return NULL;
}

static
char
gltf_image_ascii_lower(char c) {
  return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}

static
bool
gltf_image_uri_has_suffix_ci(const char * __restrict uri,
                             const char * __restrict suffix,
                             size_t                  suffixLen) {
  size_t len;
  size_t i;

  if (!uri)
    return false;

  len = strlen(uri);
  if (len < suffixLen)
    return false;

  uri += len - suffixLen;
  for (i = 0; i < suffixLen; i++) {
    if (gltf_image_ascii_lower(uri[i]) != suffix[i])
      return false;
  }

  return true;
}

static bool
gltf_image_uri_pct_encoded(const char * __restrict uri, size_t i);

static bool
gltf_image_uri_rel_safe(const char * __restrict uri);

static bool
gltf_image_uri_is_copy_source(AkImageSource * __restrict source);

static
bool
gltf_image_uri_can_preserve(GLTFExpState  * __restrict st,
                            AkImageSource * __restrict source) {
  if (!source || !source->uri)
    return false;

  if (gltf_image_uri_is_data(source->uri))
    return true;

  if (!gltf_image_uri_is_copy_source(source))
    return true;

  return (!st || !st->doc || !st->doc->inf || !st->doc->inf->dir)
         && gltf_image_uri_rel_safe(source->uri);
}

static
bool
gltf_image_source_file_exists(GLTFExpState  * __restrict st,
                              AkImageSource * __restrict source) {
  char        pathbuf[PATH_MAX];
  const char *path;
  struct stat stFile;

  path = gltf_image_source_path(st, source, pathbuf);
  if (!path
      && source
      && source->uri
      && !gltf_image_uri_has_scheme(source->uri)
      && !gltf_image_path_is_abs(source->uri))
    path = source->uri;

  return path && stat(path, &stFile) == 0 && stFile.st_size > 0;
}

bool
gltf_image_exportable(GLTFExpState * __restrict st,
                      AkImage      * __restrict image) {
  AkImageSource *source;
  const char    *mimeType;

  (void)st;

  if (!image)
    return false;

  source = gltf_image_source(image);
  if (image->data)
    return true;

  if (!source)
    return false;

  if (gltf_image_source_supported(source)) {
    mimeType = gltf_image_mime(source);
    if (mimeType && gltf_image_mime_supported(mimeType)) {
      if (source->type == AK_IMAGE_SOURCE_BUFFER
          || gltf_image_uri_can_preserve(st, source))
        return true;
      if (source->type == AK_IMAGE_SOURCE_URI
          && gltf_image_source_file_exists(st, source))
        return true;
    }

    /* Unsupported source images can still be transcoded when the image
       loader decodes them, and BMP has a small built-in fallback path. */
    if (source->type == AK_IMAGE_SOURCE_URI
        && gltf_image_uri_has_suffix_ci(source->uri, ".bmp", 4u)
        && gltf_image_source_file_exists(st, source))
      return true;

    if (ak_imageCanLoad(image))
      return true;
  }

  return false;
}

const char*
gltf_image_mime(AkImageSource * __restrict source) {
  if (!source)
    return NULL;

  if (gltf_image_mime_supported(source->mimeType))
    return source->mimeType;

  if (gltf_image_uri_is_data(source->uri))
    return gltf_image_data_uri_mime(source->uri);

  if (gltf_image_uri_has_suffix_ci(source->uri,
                                   _s_gltf_ext_png,
                                   _s_gltf_ext_png_len))
    return _s_gltf_mime_image_png;

  if (gltf_image_uri_has_suffix_ci(source->uri,
                                   _s_gltf_ext_jpg,
                                   _s_gltf_ext_jpg_len)
      || gltf_image_uri_has_suffix_ci(source->uri,
                                      _s_gltf_ext_jpeg,
                                      _s_gltf_ext_jpeg_len))
    return _s_gltf_mime_image_jpeg;

  if (gltf_image_uri_has_suffix_ci(source->uri,
                                   _s_gltf_ext_webp,
                                   _s_gltf_ext_webp_len))
    return _s_gltf_mime_image_webp;

  if (gltf_image_uri_has_suffix_ci(source->uri,
                                   _s_gltf_ext_ktx2,
                                   _s_gltf_ext_ktx2_len))
    return _s_gltf_mime_image_ktx2;

  return NULL;
}

bool
gltf_image_mime_or_uri_is(AkImageSource * __restrict source,
                          const char    * __restrict mimeType,
                          size_t                     mimeTypeLen,
                          const char    * __restrict ext,
                          size_t                     extLen) {
  return source
         && (gltf_image_cstr_eq_token(source->mimeType, mimeType, mimeTypeLen)
             || gltf_image_uri_has_suffix_ci(source->uri, ext, extLen));
}

static
const char*
gltf_image_path_basename(const char * __restrict path) {
  const char *slash;
  const char *backslash;
  const char *base;

  if (!path)
    return NULL;

  slash     = strrchr(path, '/');
  backslash = strrchr(path, '\\');
  if (slash && backslash)
    base = slash > backslash ? slash + 1 : backslash + 1;
  else if (slash)
    base = slash + 1;
  else if (backslash)
    base = backslash + 1;
  else
    base = path;

  return *base ? base : path;
}

static
bool
gltf_image_uri_rel_safe(const char * __restrict uri) {
  char   segFirst;
  char   segSecond;
  size_t segLen;
  size_t i;

  if (!uri
      || gltf_image_path_is_abs(uri)
      || gltf_image_uri_has_scheme(uri)
      || uri[0] == '/')
    return false;

  segFirst  = '\0';
  segSecond = '\0';
  segLen    = 0;
  for (i = 0; ; ) {
    unsigned char c;

    c = (unsigned char)uri[i];
    if (c == '/' || c == '\\' || c == '\0') {
      if (segLen == 2u && segFirst == '.' && segSecond == '.')
        return false;
      if (c == '\0')
        break;
      segFirst  = '\0';
      segSecond = '\0';
      segLen    = 0;
      i++;
      continue;
    }

    if (gltf_image_uri_pct_encoded(uri, i)) {
      int hi;
      int lo;

      hi = gltf_image_uri_hex((unsigned char)uri[i + 1u]);
      lo = gltf_image_uri_hex((unsigned char)uri[i + 2u]);
      c  = (unsigned char)((hi << 4) | lo);
      if (c == '\0' || c == '/' || c == '\\')
        return false;
      i += 3u;
    } else {
      i++;
    }

    if (segLen == 0)
      segFirst = (char)c;
    else if (segLen == 1u)
      segSecond = (char)c;
    segLen++;
  }

  return true;
}

static
int
gltf_image_mkdir(const char * __restrict path) {
#ifdef _WIN32
  return _mkdir(path);
#else
  return mkdir(path, 0777);
#endif
}

static
bool
gltf_image_mkdir_parent_dirs(char * __restrict path) {
  char *it;

  if (!path)
    return false;

  for (it = path; *it; it++) {
    if (*it != '/' && *it != '\\')
      continue;

    if (it == path)
      continue;

    *it = '\0';
    if (gltf_image_mkdir(path) != 0 && errno != EEXIST) {
      *it = '/';
      return false;
    }
    *it = '/';
  }

  return true;
}

static
char*
gltf_image_strdup(const char * __restrict src) {
  char  *dst;
  size_t len;

  if (!src)
    return NULL;

  len = strlen(src);
  if (len == (size_t)-1)
    return NULL;

  dst = malloc(len + 1u);
  if (!dst)
    return NULL;

  memcpy(dst, src, len + 1u);

  return dst;
}

static
bool
gltf_image_uri_unreserved(unsigned char c) {
  return (c >= 'A' && c <= 'Z')
         || (c >= 'a' && c <= 'z')
         || (c >= '0' && c <= '9')
         || c == '-'
         || c == '_'
         || c == '.'
         || c == '~';
}

static
bool
gltf_image_uri_pct_encoded(const char * __restrict uri, size_t i) {
  return uri[i] == '%'
         && uri[i + 1] != '\0'
         && uri[i + 2] != '\0'
         && gltf_image_uri_hex((unsigned char)uri[i + 1]) >= 0
         && gltf_image_uri_hex((unsigned char)uri[i + 2]) >= 0;
}

static
char*
gltf_image_escape_relative_uri(const char * __restrict uri) {
  static const char hex[] = "0123456789ABCDEF";
  char  *escaped;
  size_t len;
  size_t i;
  size_t j;

  if (!uri)
    return NULL;

  len = 0;
  for (i = 0; uri[i]; i++) {
    unsigned char c;

    c = (unsigned char)uri[i];
    if (c == '/' || gltf_image_uri_unreserved(c)) {
      if (len == (size_t)-1)
        return NULL;
      len++;
    } else if (gltf_image_uri_pct_encoded(uri, i)) {
      if (len > (size_t)-1 - 3u)
        return NULL;
      len += 3u;
      i += 2u;
    } else {
      if (len > (size_t)-1 - 3u)
        return NULL;
      len += 3u;
    }
  }

  escaped = malloc(len + 1u);
  if (!escaped)
    return NULL;

  j = 0;
  for (i = 0; uri[i]; i++) {
    unsigned char c;

    c = (unsigned char)uri[i];
    if (c == '/' || gltf_image_uri_unreserved(c)) {
      escaped[j++] = (char)c;
    } else if (gltf_image_uri_pct_encoded(uri, i)) {
      escaped[j++] = uri[i++];
      escaped[j++] = uri[i++];
      escaped[j++] = uri[i];
    } else {
      escaped[j++] = '%';
      escaped[j++] = hex[c >> 4];
      escaped[j++] = hex[c & 0x0f];
    }
  }
  escaped[j] = '\0';

  return escaped;
}

static
char*
gltf_image_indexed_basename(const char * __restrict base,
                            GLTFExpIndex            imageIndex,
                            size_t                  collisionIndex) {
  char   indexBuf[32];
  char  *uri;
  size_t baseLen;
  size_t indexLen;
  size_t totalLen;
  int    written;

  if (!base || !*base)
    return NULL;

  if (collisionIndex == 0)
    written = snprintf(indexBuf, sizeof(indexBuf), "%u", (unsigned)imageIndex);
  else
    written = snprintf(indexBuf, sizeof(indexBuf),
                       "%u_%zu",
                       (unsigned)imageIndex,
                       collisionIndex);
  if (written < 0 || (size_t)written >= sizeof(indexBuf))
    return NULL;

  baseLen  = strlen(base);
  indexLen = (size_t)written;
  if (baseLen > (size_t)-1 - indexLen - 9u)
    return NULL;

  totalLen = 6u + indexLen + 1u + baseLen;
  uri = malloc(totalLen + 1u);
  if (!uri)
    return NULL;

  memcpy(uri, "image_", 6u);
  memcpy(uri + 6u, indexBuf, indexLen);
  uri[6u + indexLen] = '_';
  for (baseLen = 0; base[baseLen]; baseLen++) {
    unsigned char c;

    c = (unsigned char)base[baseLen];
    uri[6u + indexLen + 1u + baseLen] =
      gltf_image_uri_unreserved(c) ? (char)c : '_';
  }
  uri[totalLen] = '\0';

  return uri;
}

static
char*
gltf_image_join_path(const char * __restrict dir,
                     const char * __restrict rel) {
  char  *path;
  size_t dirLen;
  size_t relLen;
  size_t totalLen;
  bool   needSlash;

  if (!dir || !rel)
    return NULL;

  dirLen = strlen(dir);
  while (dirLen > 0
         && (dir[dirLen - 1] == '/' || dir[dirLen - 1] == '\\'))
    dirLen--;

  relLen    = strlen(rel);
  needSlash = dirLen > 0;

  if (dirLen > (size_t)-1 - relLen - (needSlash ? 2u : 1u))
    return NULL;

  totalLen = dirLen + relLen + (needSlash ? 1u : 0u);
  path = malloc(totalLen + 1u);
  if (!path)
    return NULL;

  if (dirLen > 0)
    memcpy(path, dir, dirLen);
  if (needSlash)
    path[dirLen++] = '/';
  memcpy(path + dirLen, rel, relLen);
  path[totalLen] = '\0';

  return path;
}

AkImageSource*
gltf_image_external_uri_source(GLTFExpState * __restrict st,
                               GLTFExpIndex              imageIndex) {
  AkImage *image;

  if (!st
      || imageIndex >= st->images.count
      || (st->imageBufferViews
          && st->imageBufferViews[imageIndex] != GLTF_EXP_INDEX_NONE))
    return NULL;

  image = (AkImage *)st->images.items[imageIndex];
  return gltf_image_source(image);
}

static
bool
gltf_image_uri_is_copy_source(AkImageSource * __restrict source) {
  return source
         && source->uri
         && !gltf_image_uri_is_data(source->uri)
         && (gltf_image_path_is_abs(source->uri)
             || !gltf_image_uri_has_scheme(source->uri)
             || gltf_image_uri_is_file_scheme(source->uri));
}

static
bool
gltf_image_export_uri_fail(GLTFExpState * __restrict st,
                           RBTree       * __restrict uriMap) {
  size_t i;

  if (uriMap)
    rb_destroy(uriMap);

  if (st && st->imageExportUris) {
    for (i = 0; i < st->images.count; i++)
      free(st->imageExportUris[i]);

    free(st->imageExportUris);
    st->imageExportUris = NULL;
  }

  return false;
}

static
bool
gltf_image_uri_map_reserve(RBTree * __restrict uriMap,
                           char   * __restrict uri) {
  if (!uri)
    return false;

  if (rb_find(uriMap, uri))
    return true;

  rb_insert(uriMap, uri, (void *)1);

  return true;
}

static
char*
gltf_image_unique_generated_uri(RBTree      * __restrict uriMap,
                                const char  * __restrict base,
                                GLTFExpIndex             imageIndex) {
  char  *uri;
  size_t collisionIndex;

  for (collisionIndex = 0; collisionIndex < 1000000u; collisionIndex++) {
    uri = gltf_image_indexed_basename(base, imageIndex, collisionIndex);
    if (!uri)
      return NULL;

    if (!rb_find(uriMap, uri)) {
      rb_insert(uriMap, uri, (void *)1);
      return uri;
    }

    free(uri);
  }

  return NULL;
}

bool
gltf_image_prepare_export_uris(GLTFExpState * __restrict st) {
  RBTree *uriMap;
  char  **uris;
  size_t  i;

  if (!st || st->images.count == 0)
    return true;

  uris = calloc(st->images.count, sizeof(*uris));
  if (!uris)
    return false;

  st->imageExportUris = uris;

  uriMap = rb_newtree_str();
  if (!uriMap)
    return gltf_image_export_uri_fail(st, NULL);

  for (i = 0; i < st->images.count; i++) {
    AkImageSource *source;

    source = gltf_image_external_uri_source(st, (GLTFExpIndex)i);
    if (!source || source->type != AK_IMAGE_SOURCE_URI || !source->uri)
      continue;

    if (st->outDir
        && gltf_image_uri_is_copy_source(source)
        && gltf_image_uri_rel_safe(source->uri)) {
      uris[i] = gltf_image_escape_relative_uri(source->uri);
      if (!uris[i] || !gltf_image_uri_map_reserve(uriMap, uris[i])) {
        return gltf_image_export_uri_fail(st, uriMap);
      }
    }
  }

  for (i = 0; i < st->images.count; i++) {
    AkImageSource *source;

    if (uris[i])
      continue;

    source = gltf_image_external_uri_source(st, (GLTFExpIndex)i);
    if (!source || source->type != AK_IMAGE_SOURCE_URI || !source->uri)
      continue;

    if (!st->outDir || !gltf_image_uri_is_copy_source(source)) {
      uris[i] = gltf_image_strdup(source->uri);
    } else if (gltf_image_uri_rel_safe(source->uri)) {
      uris[i] = gltf_image_escape_relative_uri(source->uri);
    } else {
      uris[i] = gltf_image_unique_generated_uri(
                  uriMap,
                  gltf_image_path_basename(source->uri),
                  (GLTFExpIndex)i);
    }

    if (!uris[i]) {
      return gltf_image_export_uri_fail(st, uriMap);
    }

    if (gltf_image_uri_is_copy_source(source)
        && !gltf_image_uri_map_reserve(uriMap, uris[i])) {
      return gltf_image_export_uri_fail(st, uriMap);
    }
  }

  rb_destroy(uriMap);

  return true;
}

bool
gltf_image_copy_export_uri(GLTFExpState  * __restrict st,
                           AkImageSource * __restrict source,
                           const char    * __restrict exportUri) {
  char        pathbuf[PATH_MAX];
  char        relbuf[PATH_MAX];
  const char *srcPath;
  const char *dstRel;
  char       *dstPath;
  bool        ok;

  if (!source || !source->uri || !exportUri)
    return false;

  if (!st->outDir || !gltf_image_uri_is_copy_source(source))
    return true;

  srcPath = gltf_image_source_path(st, source, pathbuf);
  if (!srcPath
      && source->uri
      && !gltf_image_uri_has_scheme(source->uri)
      && !gltf_image_path_is_abs(source->uri)) {
    struct stat stFile;

    if (stat(source->uri, &stFile) == 0 && stFile.st_size > 0)
      srcPath = source->uri;
  }
  if (!srcPath)
    return true;

  dstRel = exportUri;
  if (strchr(exportUri, '%')) {
    if (!gltf_image_uri_decode_path(exportUri, relbuf, sizeof(relbuf))
        || !gltf_image_uri_rel_safe(relbuf))
      return false;
    dstRel = relbuf;
  }

  dstPath = gltf_image_join_path(st->outDir, dstRel);
  if (!dstPath)
    return false;

  ok = gltf_image_mkdir_parent_dirs(dstPath)
       && ak_copyfile(srcPath, dstPath);
  free(dstPath);

  return ok;
}
