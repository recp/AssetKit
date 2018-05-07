/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../memory_common.h"

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

AK_EXPORT
void
ak_libInsertInto(AkLibItem *lib,
                 void      *item,
                 int32_t    prevOffset,
                 int32_t    nextOffset) {
  char *libChld, *lastLibChld;

  libChld = lastLibChld = lib->chld;
  while (libChld) {
    libChld = *(char **)(lastLibChld + nextOffset);
    if (libChld)
      lastLibChld = libChld;
  }

  if (lastLibChld)
    *(void **)(lastLibChld + nextOffset) = item;
  else
    lib->chld = item;

  if (prevOffset > -1)
    *(void **)((char *)item + prevOffset) = lastLibChld;
}

AK_EXPORT
AkLibItem*
ak_libFirstOrCreat(AkDoc * __restrict doc,
                   uint32_t           itemOffset) {
  AkHeap    *heap;
  AkLibItem *lib;

  heap = ak_heap_getheap(doc);
  lib  = *(void **)((char *)&doc->lib + itemOffset);
  if (lib)
    return lib;

  lib = ak_heap_calloc(heap,
                       doc,
                       sizeof(*lib));
  doc->lib.images = lib;
  return lib;
}

AK_EXPORT
AkLibItem*
ak_libImageFirstOrCreat(AkDoc * __restrict doc) {
  return ak_libFirstOrCreat(doc, offsetof(AkLib, images));
}
