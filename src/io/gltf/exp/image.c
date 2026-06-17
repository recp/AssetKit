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
#include "../../common/path.h"
#include "../../common/string.h"
#include "../../common/util.h"
#include "../../common/uri.h"
#include "../../../image/export.h"

#include <ak/path.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#  define PATH_MAX 260
#endif

AkImageSource*
gltf_image_source(AkImage * __restrict image) {
  return ak_imageSource(image);
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

const char*
gltf_image_source_path(GLTFExpState  * __restrict st,
                       AkImageSource * __restrict source,
                       char          * __restrict pathbuf) {
  const char *filePath;
  char        relbuf[PATH_MAX];

  if (!source
      || !source->uri
      || io_uri_has_prefix(source->uri, _s_gltf_b64d, _s_gltf_b64d_len))
    return NULL;

  if (source->resolvedPath)
    return source->resolvedPath;

  filePath = io_uri_file_path(source->uri,
                              _s_gltf_file_uri,
                              _s_gltf_file_uri_len);
  if (filePath) {
    if (strchr(filePath, '%')) {
      if (!io_uri_decode_path(filePath, pathbuf, PATH_MAX))
        return NULL;
      return pathbuf;
    }
    return filePath;
  }

  if (io_uri_has_scheme(source->uri))
    return NULL;

  if (st->doc && st->doc->inf && st->doc->inf->dir) {
    const char *uriPath;

    uriPath = source->uri;
    if (strchr(uriPath, '%')) {
      if (!io_uri_decode_path(uriPath, relbuf, sizeof(relbuf)))
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

  if (!io_uri_has_prefix(uri, _s_gltf_b64d, _s_gltf_b64d_len))
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
gltf_image_uri_rel_safe(const char * __restrict uri);

static bool
gltf_image_uri_is_copy_source(AkImageSource * __restrict source);

static
bool
gltf_image_uri_can_preserve(GLTFExpState  * __restrict st,
                            AkImageSource * __restrict source) {
  if (!source || !source->uri)
    return false;

  if (io_uri_has_prefix(source->uri, _s_gltf_b64d, _s_gltf_b64d_len))
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
      && !io_uri_has_scheme(source->uri)
      && !io_path_is_abs_drive_colon(source->uri))
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

  if (io_uri_has_prefix(source->uri, _s_gltf_b64d, _s_gltf_b64d_len))
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
bool
gltf_image_uri_rel_safe(const char * __restrict uri) {
  char   segFirst;
  char   segSecond;
  size_t segLen;
  size_t i;

  if (!uri
      || io_path_is_abs_drive_colon(uri)
      || io_uri_has_scheme(uri)
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

    if (io_uri_pct_encoded(uri, i)) {
      int hi;
      int lo;

      hi = io_uri_hex_digit((unsigned char)uri[i + 1u]);
      lo = io_uri_hex_digit((unsigned char)uri[i + 2u]);
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
      io_uri_unreserved(c) ? (char)c : '_';
  }
  uri[totalLen] = '\0';

  return uri;
}

static
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
         && !io_uri_has_prefix(source->uri, _s_gltf_b64d, _s_gltf_b64d_len)
         && (io_path_is_abs_drive_colon(source->uri)
             || !io_uri_has_scheme(source->uri)
             || io_uri_has_prefix(source->uri,
                                  _s_gltf_file_uri,
                                  _s_gltf_file_uri_len));
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

    if (io_rb_insert_absent_key(uriMap, uri))
      return uri;

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
      uris[i] = io_uri_escape_dup(source->uri, true, true);
      if (!uris[i] || !io_rb_reserve_key(uriMap, uris[i])) {
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
      uris[i] = io_strdup(source->uri);
    } else if (gltf_image_uri_rel_safe(source->uri)) {
      uris[i] = io_uri_escape_dup(source->uri, true, true);
    } else {
      uris[i] = gltf_image_unique_generated_uri(
                  uriMap,
                  io_path_basename_nonempty(source->uri),
                  (GLTFExpIndex)i);
    }

    if (!uris[i]) {
      return gltf_image_export_uri_fail(st, uriMap);
    }

    if (gltf_image_uri_is_copy_source(source)
        && !io_rb_reserve_key(uriMap, uris[i])) {
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
      && !io_uri_has_scheme(source->uri)
      && !io_path_is_abs_drive_colon(source->uri)) {
    struct stat stFile;

    if (stat(source->uri, &stFile) == 0 && stFile.st_size > 0)
      srcPath = source->uri;
  }
  if (!srcPath)
    return true;

  dstRel = exportUri;
  if (strchr(exportUri, '%')) {
    if (!io_uri_decode_path(exportUri, relbuf, sizeof(relbuf))
        || !gltf_image_uri_rel_safe(relbuf))
      return false;
    dstRel = relbuf;
  }

  dstPath = io_path_join_dup_trim_dir(st->outDir, dstRel);
  if (!dstPath)
    return false;

  ok = io_path_mkdir_parent_dirs(dstPath, true)
       && ak_copyfile(srcPath, dstPath);
  free(dstPath);

  return ok;
}
