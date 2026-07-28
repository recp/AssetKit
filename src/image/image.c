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

#include "../common.h"
#include "../string_fast.h"
#include "export.h"
#include "../io/common/uri.h"
#include "../../include/ak/bbox.h"
#include "../../include/ak/path.h"
#include <limits.h>
#include <string.h>

#ifdef _MSC_VER
#  ifndef PATH_MAX
#    define PATH_MAX 260
#  endif
#endif

typedef struct AkImageConf {
  AkImageLoadFromFileFn   loadFromFile;
  AkImageLoadFromMemoryFn loadFromMemory;
} AkImageConf;

static AkImageConf ak__img_conf = {0};

AK_EXPORT
void
ak_imageInitLoader(AkImageLoadFromFileFn   fromFile,
                   AkImageLoadFromMemoryFn fromMemory) {
  ak__img_conf.loadFromFile         = fromFile;
  ak__img_conf.loadFromMemory       = fromMemory;
}

AK_EXPORT
const char*
ak_imageResolvePath(AkImage * __restrict image) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImageSource *source;
  char           pathbuf[PATH_MAX];
  char           uribuf[PATH_MAX];
  const char    *uriPath;
  const char    *path;

  if (!image)
    return NULL;

  source = ak_imageSource(image);
  if (!source || source->type != AK_IMAGE_SOURCE_URI || !source->uri)
    return NULL;
  if (source->resolvedPath && *source->resolvedPath)
    return source->resolvedPath;

  uriPath = source->uri;
  if (ak_str_has_char_fast(uriPath, '%')) {
    if (!io_uri_decode_path(uriPath, uribuf, sizeof(uribuf)))
      return NULL;
    uriPath = uribuf;
  }

  heap = ak_heap_getheap(image);
  doc  = ak_heap_data(heap);
  path = ak_fullpathn(doc, uriPath, pathbuf, sizeof(pathbuf));
  if (!path)
    path = uriPath;

  source->resolvedPath = ak_strdup(source, path);
  return source->resolvedPath;
}

AK_HIDE
bool
ak_imageCanLoad(AkImage * __restrict image) {
  AkImageSource *source;

  if (!image)
    return false;
  if (image->data)
    return true;

  source = ak_imageSource(image);
  if (!source)
    return false;

  switch (source->type) {
    case AK_IMAGE_SOURCE_URI:
      return source->uri && ak__img_conf.loadFromFile;
    case AK_IMAGE_SOURCE_BUFFER:
      return source->buffer
             && source->buffer->data
             && ak__img_conf.loadFromMemory;
    default:
      break;
  }

  return false;
}

AK_EXPORT
void
ak_imageLoad(AkImage * __restrict image) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImageSource *source;
  bool           flipImage;

  if (image->data)
    return;

  heap      = ak_heap_getheap(image);
  doc       = ak_heap_data(heap);
  flipImage = false;

  /* glTF uses top-left as origin */
  if (doc && doc->inf && doc->inf->flipImage) {
    flipImage = ak_opt_get(AK_OPT_IMAGE_LOAD_FLIP_VERTICALLY);
  }

  source = ak_imageSource(image);
  if (source) {
    switch (source->type) {
    case AK_IMAGE_SOURCE_URI: {
      const char *path;

      if (!source->uri || !ak__img_conf.loadFromFile)
        return;

      path = ak_imageResolvePath(image);
      if (!path)
        return;
      image->data          = ak__img_conf.loadFromFile(heap, image, path, flipImage);
      break;
    }
    case AK_IMAGE_SOURCE_BUFFER:
      if (!source->buffer || !source->buffer->data || !ak__img_conf.loadFromMemory)
        return;

      image->data = ak__img_conf.loadFromMemory(heap, image, source->buffer, flipImage);
      break;
    case AK_IMAGE_SOURCE_HEX:
      /* TODO: */
      break;
    case AK_IMAGE_SOURCE_NONE:
      break;
    }
  }
}
