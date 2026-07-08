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
#include "io.h"
#include "../../../image/export.h"
#include "../strpool.h"
#include "../../common/path.h"
#include "../../common/string.h"
#include "../../common/text_number.h"
#include "../../common/util.h"
#include "../../common/uri.h"
#include "../../../string_fast.h"

#include <ak/path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
bool
dae_uri_is_data_scheme(const char * __restrict uri) {
  return io_uri_has_prefix(uri, _s_dae_data, _s_dae_data_len)
         && uri[_s_dae_data_len] == ':';
}

AK_HIDE
bool
dae_prepare_extra_image(DAEExpState * __restrict st,
                        AkImage     * __restrict image) {
  if (!image)
    return true;

  return dae_prepare_extra_object(st->images,
                                  &st->imageCount,
                                  &st->extraImages,
                                  &st->lastExtraImage,
                                  image);
}

AK_HIDE
bool
dae_prepare_texture_image(DAEExpState  * __restrict st,
                          AkTextureRef * __restrict texref) {
  AkTexture *tex;

  tex = texref ? texref->texture : NULL;
  return !tex || !tex->image || dae_prepare_extra_image(st, tex->image);
}

static
const char*
dae_image_mime_ext(AkImageSource * __restrict source) {
  const char *mime;

  mime = source ? source->mimeType : NULL;
  if (ak_str_eq_cstr_fast(mime, "image/png", sizeof("image/png") - 1u))
    return ".png";
  if (ak_str_eq_cstr_fast(mime, "image/jpeg", sizeof("image/jpeg") - 1u))
    return ".jpg";
  if (ak_str_eq_cstr_fast(mime, "image/webp", sizeof("image/webp") - 1u))
    return ".webp";
  if (ak_str_eq_cstr_fast(mime, "image/ktx2", sizeof("image/ktx2") - 1u))
    return ".ktx2";

  return ".bin";
}

static
char*
dae_image_buffer_uri(AkImageSource * __restrict source,
                     uint32_t                   imageIdx,
                     size_t                     collisionIdx) {
  const char *ext;
  char        indexBuf[32];
  char       *indexEnd;
  char       *uri;
  size_t      extLen;
  size_t      indexLen;
  size_t      size;

  ext      = dae_image_mime_ext(source);
  extLen   = strlen(ext);
  indexEnd = ak_io_text_format_uint32(indexBuf, imageIdx);
  if (collisionIdx != 0) {
    *indexEnd++ = '_';
    indexEnd = ak_io_text_format_uint64(indexEnd, (uint64_t)collisionIdx);
  }
  indexLen = (size_t)(indexEnd - indexBuf);

  if (indexLen > (size_t)-1 - extLen - 7u)
    return NULL;

  size = 6u + indexLen + extLen + 1u;
  uri  = malloc(size);
  if (!uri)
    return NULL;

  memcpy(uri, "image_", 6u);
  memcpy(uri + 6u, indexBuf, indexLen);
  memcpy(uri + 6u + indexLen, ext, extLen + 1u);

  return uri;
}

static
char*
dae_image_unique_buffer_uri(RBTree        * __restrict uriMap,
                            AkImageSource * __restrict source,
                            uint32_t                   imageIdx) {
  char  *generated;
  size_t collisionIdx;

  for (collisionIdx = 0; collisionIdx < 1000000u; collisionIdx++) {
    generated = dae_image_buffer_uri(source, imageIdx, collisionIdx);
    if (!generated)
      return NULL;

    if (io_rb_insert_absent_key(uriMap, generated))
      return generated;

    free(generated);
  }

  return NULL;
}

static
const char*
dae_image_uri(AkImage * __restrict image) {
  AkImageSource *source;

  for (source = ak_imageSource(image); source; source = source->next) {
    if (source->type == AK_IMAGE_SOURCE_URI && source->uri)
      return source->uri;
  }

  source = ak_imageSource(image);
  if (source) {
    if (source->uri)
      return source->uri;
    if (source->resolvedPath)
      return source->resolvedPath;
  }

  return NULL;
}

static
const char*
dae_image_source_path(DAEExpState  * __restrict st,
                      AkImageSource * __restrict source,
                      char          * __restrict pathbuf,
                      size_t                     pathbufCap) {
  const char *filePath;

  if (!source || !source->uri || dae_uri_is_data_scheme(source->uri))
    return NULL;

  if (source->resolvedPath)
    return source->resolvedPath;

  filePath = io_uri_file_path(source->uri,
                              _s_dae_file_uri,
                              _s_dae_file_uri_len);
  if (filePath) {
    if (ak_str_has_char_fast(filePath, '%')) {
      if (!io_uri_decode_path(filePath, pathbuf, pathbufCap))
        return NULL;
      return pathbuf;
    }
    return filePath;
  }

  if (io_uri_has_scheme(source->uri))
    return NULL;

  if (st->doc
      && st->doc->inf
      && st->doc->inf->dir
      && st->doc->inf->dir[0]) {
    if (io_path_join_buf(st->doc->inf->dir,
                         source->uri,
                         pathbuf,
                         pathbufCap))
      return pathbuf;
  }

  return source->uri;
}

static
bool
dae_image_uri_is_copy_source(AkImageSource * __restrict source) {
  return source
         && source->type == AK_IMAGE_SOURCE_URI
         && source->uri
         && !dae_uri_is_data_scheme(source->uri)
         && (io_path_is_abs_drive_colon(source->uri)
             || !io_uri_has_scheme(source->uri)
             || io_uri_has_prefix(source->uri,
                                  _s_dae_file_uri,
                                  _s_dae_file_uri_len));
}

static
bool
dae_image_copy_uri(DAEExpState  * __restrict st,
                   AkImageSource * __restrict source,
                   const char    * __restrict uri) {
  char        srcbuf[4096];
  char        dstbuf[4096];
  char       *dstPath;
  const char *srcPath;
  bool        ok;
  bool        heapPath;

  if (!st->outDir
      || !source
      || !dae_image_uri_is_copy_source(source)
      || !uri
      || dae_uri_is_data_scheme(uri)
      || io_uri_has_scheme(uri)
      || io_path_is_abs_drive_colon(uri)
      || !dae_uri_rel_safe(uri))
    return true;

  srcPath = dae_image_source_path(st, source, srcbuf, sizeof(srcbuf));
  if (!srcPath)
    return true;

  heapPath = false;
  if (io_path_join_buf(st->outDir, uri, dstbuf, sizeof(dstbuf))) {
    dstPath = dstbuf;
  } else {
    dstPath  = io_path_join_dup(st->outDir, uri);
    heapPath = true;
  }
  if (!dstPath)
    return false;

  ok = io_path_mkdir_parent_dirs(dstPath, false)
       && ak_copyfile(srcPath, dstPath);
  if (heapPath)
    free(dstPath);

  return ok;
}

static
bool
dae_image_write_buffer(DAEExpState  * __restrict st,
                       AkImageSource * __restrict source,
                       const char    * __restrict uri) {
  char  dstbuf[4096];
  char *dstPath;
  bool  ok;
  bool  heapPath;

  if (!source || source->type != AK_IMAGE_SOURCE_BUFFER)
    return true;

  if (!st->outDir
      || !source->buffer
      || !source->buffer->data
      || source->buffer->length == 0
      || !uri
      || dae_uri_is_data_scheme(uri)
      || io_uri_has_scheme(uri)
      || io_path_is_abs_drive_colon(uri)
      || !dae_uri_rel_safe(uri))
    return false;

  heapPath = false;
  if (io_path_join_buf(st->outDir, uri, dstbuf, sizeof(dstbuf))) {
    dstPath = dstbuf;
  } else {
    dstPath  = io_path_join_dup(st->outDir, uri);
    heapPath = true;
  }
  if (!dstPath)
    return false;

  ok = io_path_mkdir_parent_dirs(dstPath, false)
       && dae_write_file_bytes(dstPath,
                               source->buffer->data,
                               source->buffer->length);
  if (heapPath)
    free(dstPath);

  return ok;
}

static
const char*
dae_uri_basename(const char * __restrict uri) {
  const char *base;

  if (!uri || !*uri)
    return NULL;

  base = io_path_basename(uri);

  if (!*base
      || (base[0] == '.'
          && (base[1] == '\0'
              || (base[1] == '.' && base[2] == '\0'))))
    return NULL;

  return base;
}

static
bool
dae_image_needs_uri_rewrite(const char * __restrict uri) {
  return uri
         && !dae_uri_is_data_scheme(uri)
         && !io_uri_has_scheme(uri)
         && !io_path_is_abs_drive_colon(uri)
         && !dae_uri_rel_safe(uri);
}

static
bool
dae_image_rewrite_uri(uint32_t                 imageIdx,
                      const char * __restrict  uri,
                      char       * __restrict  out,
                      size_t                   outCap) {
  const char *base;
  char        indexBuf[16];
  char       *indexEnd;
  char       *p;
  size_t      baseLen;
  size_t      indexLen;
  size_t      totalLen;

  if (!out || outCap == 0)
    return false;

  base = dae_uri_basename(uri);
  if (!base)
    base = "image";

  baseLen  = strlen(base);
  indexEnd = ak_io_text_format_uint32(indexBuf, imageIdx);
  indexLen = (size_t)(indexEnd - indexBuf);
  if (baseLen > (size_t)-1 - indexLen - 8u)
    return false;

  totalLen = 6u + indexLen + 1u + baseLen;
  if (totalLen >= outCap)
    return false;

  p = out;
  memcpy(p, "image_", 6u);
  p += 6u;
  memcpy(p, indexBuf, indexLen);
  p += indexLen;
  *p++ = '_';
  memcpy(p, base, baseLen + 1u);

  return true;
}

static
char*
dae_image_indexed_basename(const char * __restrict base,
                           uint32_t                imageIdx,
                           size_t                  collisionIdx) {
  char   indexBuf[32];
  char  *indexEnd;
  char  *uri;
  size_t baseLen;
  size_t indexLen;
  size_t size;

  if (!base || !*base)
    base = "image";

  baseLen  = strlen(base);
  indexEnd = ak_io_text_format_uint32(indexBuf, imageIdx);
  if (collisionIdx != 0) {
    *indexEnd++ = '_';
    indexEnd = ak_io_text_format_uint64(indexEnd, (uint64_t)collisionIdx);
  }
  indexLen = (size_t)(indexEnd - indexBuf);
  if (baseLen > (size_t)-1 - indexLen - 8u)
    return NULL;

  size = 6u + indexLen + 1u + baseLen + 1u;
  uri  = malloc(size);
  if (!uri)
    return NULL;

  memcpy(uri, "image_", 6u);
  memcpy(uri + 6u, indexBuf, indexLen);
  uri[6u + indexLen] = '_';
  memcpy(uri + 6u + indexLen + 1u, base, baseLen + 1u);

  return uri;
}

static
char*
dae_image_unique_generated_uri(RBTree      * __restrict uriMap,
                               const char  * __restrict uri,
                               uint32_t                 imageIdx) {
  const char *base;
  char       *generated;
  size_t      collisionIdx;

  base = dae_uri_basename(uri);
  for (collisionIdx = 0; collisionIdx < 1000000u; collisionIdx++) {
    generated = dae_image_indexed_basename(base, imageIdx, collisionIdx);
    if (!generated)
      return NULL;

    if (io_rb_insert_absent_key(uriMap, generated))
      return generated;

    free(generated);
  }

  return NULL;
}

static
bool
dae_image_export_uri_fail(DAEExpState * __restrict st,
                          RBTree      * __restrict uriMap) {
  uint32_t i;

  if (uriMap)
    rb_destroy(uriMap);

  if (st && st->imageExportUris) {
    for (i = 0; i < st->imageCount; i++)
      free(st->imageExportUris[i]);

    free(st->imageExportUris);
    st->imageExportUris = NULL;
  }

  return false;
}

AK_HIDE
bool
dae_prepare_image_export_uris(DAEExpState * __restrict st) {
  RBTree          *uriMap;
  DAEExpObjectRef *objRef;
  AkImage         *image;
  uint32_t         idx;

  if (!st || st->imageCount == 0)
    return true;

  st->imageExportUris = calloc(st->imageCount, sizeof(*st->imageExportUris));
  if (!st->imageExportUris)
    return false;

  uriMap = rb_newtree_str();
  if (!uriMap)
    return dae_image_export_uri_fail(st, NULL);

  idx = 0;
  if (st->imageRefsOnly) {
    for (objRef = st->extraImages; objRef; objRef = objRef->next, idx++) {
      AkImageSource *source;

      image  = objRef->object;
      source = ak_imageSource(image);
      if (!dae_image_uri_is_copy_source(source)
          || !dae_uri_rel_safe(source->uri)
          || dae_uri_is_data_scheme(source->uri)
          || io_uri_has_scheme(source->uri)
          || io_path_is_abs_drive_colon(source->uri))
        continue;

      st->imageExportUris[idx] = io_strdup(source->uri);
      if (!st->imageExportUris[idx]
          || !io_rb_reserve_key(uriMap, st->imageExportUris[idx]))
        return dae_image_export_uri_fail(st, uriMap);
    }
  } else {
    for (image = st->doc->lib.images.first; image; image = image->next, idx++) {
      AkImageSource *source;

      source = ak_imageSource(image);
      if (!dae_image_uri_is_copy_source(source)
          || !dae_uri_rel_safe(source->uri)
          || dae_uri_is_data_scheme(source->uri)
          || io_uri_has_scheme(source->uri)
          || io_path_is_abs_drive_colon(source->uri))
        continue;

      st->imageExportUris[idx] = io_strdup(source->uri);
      if (!st->imageExportUris[idx]
          || !io_rb_reserve_key(uriMap, st->imageExportUris[idx]))
        return dae_image_export_uri_fail(st, uriMap);
    }
  }

  idx = 0;
  if (st->imageRefsOnly) {
    for (objRef = st->extraImages; objRef; objRef = objRef->next, idx++) {
      AkImageSource *source;

      if (st->imageExportUris[idx])
        continue;

      image  = objRef->object;
      source = ak_imageSource(image);
      if (!source)
        continue;

      if (source->type == AK_IMAGE_SOURCE_BUFFER) {
        st->imageExportUris[idx] =
          dae_image_unique_buffer_uri(uriMap, source, idx);
      } else if (source->type != AK_IMAGE_SOURCE_URI || !source->uri) {
        continue;
      } else if (!st->outDir || !dae_image_uri_is_copy_source(source)) {
        st->imageExportUris[idx] = io_strdup(source->uri);
      } else if (dae_uri_rel_safe(source->uri)
                 && !dae_uri_is_data_scheme(source->uri)
                 && !io_uri_has_scheme(source->uri)
                 && !io_path_is_abs_drive_colon(source->uri)) {
        st->imageExportUris[idx] = io_strdup(source->uri);
        if (st->imageExportUris[idx])
          io_rb_reserve_key(uriMap, st->imageExportUris[idx]);
      } else {
        st->imageExportUris[idx] =
          dae_image_unique_generated_uri(uriMap, source->uri, idx);
      }

      if (!st->imageExportUris[idx])
        return dae_image_export_uri_fail(st, uriMap);
    }

    rb_destroy(uriMap);
    return true;
  }

  for (image = st->doc->lib.images.first; image; image = image->next, idx++) {
    AkImageSource *source;

    if (st->imageExportUris[idx])
      continue;

    source = ak_imageSource(image);
    if (!source)
      continue;

    if (source->type == AK_IMAGE_SOURCE_BUFFER) {
      st->imageExportUris[idx] =
        dae_image_unique_buffer_uri(uriMap, source, idx);
    } else if (source->type != AK_IMAGE_SOURCE_URI || !source->uri) {
      continue;
    } else if (!st->outDir || !dae_image_uri_is_copy_source(source)) {
      st->imageExportUris[idx] = io_strdup(source->uri);
    } else if (dae_uri_rel_safe(source->uri)
               && !dae_uri_is_data_scheme(source->uri)
               && !io_uri_has_scheme(source->uri)
               && !io_path_is_abs_drive_colon(source->uri)) {
      st->imageExportUris[idx] = io_strdup(source->uri);
      if (st->imageExportUris[idx])
        io_rb_reserve_key(uriMap, st->imageExportUris[idx]);
    } else {
      st->imageExportUris[idx] =
        dae_image_unique_generated_uri(uriMap, source->uri, idx);
    }

    if (!st->imageExportUris[idx])
      return dae_image_export_uri_fail(st, uriMap);
  }
  rb_destroy(uriMap);
  return true;
}

AK_HIDE
void
dae_write_image(DAEExpState * __restrict st,
                AkImage     * __restrict image,
                uint32_t                 imageIdx) {
  DAEExpWriter  *w;
  AkImageSource *source;
  const char    *uri;
  const char    *exportUri;
  char           rewrittenUri[512];
  bool           preparedUri;

  w      = &st->w;
  source = ak_imageSource(image);
  uri    = dae_image_uri(image);
  preparedUri = st->imageExportUris
                && imageIdx < st->imageCount
                && st->imageExportUris[imageIdx] != NULL;
  exportUri   = preparedUri ? st->imageExportUris[imageIdx] : NULL;
  if (!exportUri)
    exportUri = uri;
  if (!exportUri || exportUri[0] == '\0') {
    w->result = AK_EINVAL;
    return;
  }

  if (!preparedUri && dae_image_needs_uri_rewrite(uri)) {
    if (!dae_image_rewrite_uri(imageIdx,
                               uri,
                               rewrittenUri,
                               sizeof(rewrittenUri))) {
      w->result = AK_ERR;
      return;
    }
    exportUri = rewrittenUri;
  }

  if (source && !dae_image_copy_uri(st, source, exportUri)) {
    w->result = AK_ERR;
    return;
  }
  if (source && !dae_image_write_buffer(st, source, exportUri)) {
    w->result = AK_ERR;
    return;
  }

  dae_w_lit(w, "<image id=\"");
  dae_w_id(w, DAE_EXP_NAME(image), imageIdx);
  if (image && image->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, image->name, true);
  }
  dae_w_lit(w, "\"><init_from>");
  if (st->useCollada150)
    dae_w_lit(w, "<ref>");
  dae_w_xml(w, exportUri ? exportUri : "", false);
  if (st->useCollada150)
    dae_w_lit(w, "</ref>");
  dae_w_lit(w, "</init_from>");
  dae_write_extra(w, image ? image->extra : NULL);
  dae_w_lit(w, "</image>");
}
