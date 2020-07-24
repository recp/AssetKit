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
#include "../../include/ak/bbox.h"
#include "../../include/ak/path.h"
#include <limits.h>

#ifdef _MSC_VER
#  ifndef PATH_MAX
#    define PATH_MAX MAX_PATH
#  endif
#endif

typedef struct AkImageConf {
  void* (*loadFromFile)(const char * __restrict path,
                        int        * __restrict width,
                        int        * __restrict height,
                        int        * __restrict components);
  void* (*loadFromMemory)(const char * __restrict data,
                          size_t                  len,
                          int        * __restrict width,
                          int        * __restrict height,
                          int        * __restrict components);
  void (*flipVerticallyOnLoad)(bool flip);
} AkImageConf;

static AkImageConf ak__img_conf = {0};

AK_EXPORT
void
ak_imageInitLoader(AkImageLoadFromFileFn       * __restrict fromFile,
                   AkImageLoadFromMemoryFn     * __restrict fromMemory,
                   AkImageFlipVerticallyOnLoad * __restrict flipper) {
  ak__img_conf.loadFromFile         = fromFile;
  ak__img_conf.loadFromMemory       = fromMemory;
  ak__img_conf.flipVerticallyOnLoad = flipper;
}

AK_EXPORT
void
ak_imageLoad(AkImage * __restrict image) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImageData   *idata;
  unsigned char *data;
  bool           flipImage;

  if (image->data)
    return;

  idata = NULL;
  data  = NULL;
  heap  = ak_heap_getheap(image);
  doc   = ak_heap_data(heap);

  /* glTF uses top-left as origin */
  if (doc->inf->flipImage) {
    flipImage = ak_opt_get(AK_OPT_IMAGE_LOAD_FLIP_VERTICALLY);
    ak__img_conf.flipVerticallyOnLoad(flipImage);
  }

  if (image->initFrom) {
    AkInitFrom *initFrom;
    int         x, y, ch;

    initFrom = image->initFrom;
    if (initFrom->ref) {
      char        pathbuf[PATH_MAX];
      const char *path;

      if (!ak__img_conf.loadFromFile)
        return;

      path = ak_fullpath(doc, initFrom->ref, pathbuf);
      data = ak__img_conf.loadFromFile(path, &x, &y, &ch);
      if (!data)
        return;

      idata = ak_heap_calloc(heap, image, sizeof(*idata));
      idata->width  = x;
      idata->height = y;
      idata->comp   = ch;
      idata->data   = data;

      image->data   = idata;
    } else if (initFrom->buff && initFrom->buff->data) {
      if (!ak__img_conf.loadFromMemory)
        return;

      data = ak__img_conf.loadFromMemory(initFrom->buff->data,
                                         (int)initFrom->buff->length,
                                         &x, &y, &ch);

      if (!data)
        return;

      idata         = ak_heap_calloc(heap, image, sizeof(*idata));
      idata->width  = x;
      idata->height = y;
      idata->comp   = ch;
      idata->data   = data;
      image->data   = idata;
    } else if (initFrom->hex) {
      /* TODO: */
    }
  }
}
