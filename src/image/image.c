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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define STBI_MALLOC(sz)           ak_malloc(NULL, sz)
#define STBI_REALLOC(p,newsz)     ak_realloc(NULL, p, newsz)
#define STBI_FREE(p)              ak_free(p)

#define STBIDEF static inline
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma GCC diagnostic pop

#ifdef _MSC_VER
#  ifndef PATH_MAX
#    define PATH_MAX MAX_PATH
#  endif
#endif

void
ak_imageLoad(AkImage * __restrict image) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImageData   *idata;
  unsigned char *data;
  bool           flipImage;

  if (image->data)
    return;

  idata  = NULL;
  data   = NULL;
  heap   = ak_heap_getheap(image);
  doc    = ak_heap_data(heap);

  /* glTF uses top-left as origin */
  if (doc->inf->flipImage) {
    flipImage = ak_opt_get(AK_OPT_IMAGE_LOAD_FLIP_VERTICALLY);
    stbi_set_flip_vertically_on_load(flipImage);
  }

  if (image->initFrom) {
    AkInitFrom *initFrom;
    int         x, y, ch;

    initFrom = image->initFrom;
    if (initFrom->ref) {
      char        pathbuf[PATH_MAX];
      const char *path;

      path = ak_fullpath(doc, initFrom->ref, pathbuf);
      data = stbi_load(path, &x, &y, &ch, 0);
      if (!data)
        return;

      idata = ak_heap_calloc(heap, image, sizeof(*idata));
      idata->width  = x;
      idata->height = y;
      idata->comp   = ch;
      idata->data   = data;

      image->data   = idata;
    } else if (initFrom->buff && initFrom->buff->data) {
      data = stbi_load_from_memory(initFrom->buff->data,
                                   (int)initFrom->buff->length,
                                   &x, &y, &ch, 0);
      if (!data)
        return;

      idata = ak_heap_calloc(heap, image, sizeof(*idata));
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
