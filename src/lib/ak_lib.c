/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

AK_EXPORT
AkResult
ak_libAddCamera(AkDoc    * __restrict doc,
                AkCamera * __restrict cam) {
  AkHeap    *heap;
  AkLibItem *libItem;
  AkCamera  *cami;

  heap    = ak_heap_getheap(doc);
  libItem = doc->lib.cameras;
  if (!libItem) {
    libItem = ak_heap_calloc(heap, doc, sizeof(*libItem));
    doc->lib.cameras = libItem;
    libItem->count   = 1;
  }

  cami = libItem->chld;
  if (cami) {
    cam->next = cami;
  }

  libItem->chld = cam;

  return AK_OK;
}

AK_EXPORT
AkResult
ak_libAddLight(AkDoc   * __restrict doc,
               AkLight * __restrict light) {
  AkHeap    *heap;
  AkLibItem *libItem;
  AkLight   *lighti;

  heap    = ak_heap_getheap(doc);
  libItem = doc->lib.lights;
  if (!libItem) {
    libItem = ak_heap_calloc(heap, doc, sizeof(*libItem));
    doc->lib.lights = libItem;
    libItem->count   = 1;
  }

  lighti = libItem->chld;
  if (lighti) {
    light->next = lighti;
  }

  libItem->chld = light;

  return AK_OK;
}
