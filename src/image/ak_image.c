/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../../include/ak-bbox.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define STBI_MALLOC(sz)           ak_malloc(NULL, sz)
#define STBI_REALLOC(p,newsz)     ak_realloc(NULL, p, newsz)
#define STBI_FREE(p)              ak_free(p)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma GCC diagnostic pop

void*
ak_imageLoad(AkImage * __restrict image) {
  AkHeap        *heap;
  unsigned char *data;

  data = NULL;
  heap = ak_heap_getheap(image);
  if (image->initFrom) {
    AkInitFrom *initFrom;
    int x, y, ch;

    initFrom = image->initFrom;
    if (initFrom->ref) {
      data = stbi_load(initFrom->ref, &x, &y, &ch, 0);
    } else if (initFrom->hex) {
      /* TODO: */
    }

    ak_heap_setpm(data, image);
  }
  return data;
}
