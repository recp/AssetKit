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

AK_EXPORT
AkResult
ak_libAddCamera(AkDoc * __restrict doc, AkCamera * __restrict cam) {
  AkHeap    *heap;
  AkLibrary *libItem;
  AkCamera  *cami;

  heap    = ak_heap_getheap(doc);
  libItem = doc->lib.cameras;
  if (!libItem) {
    libItem = ak_heap_calloc(heap, doc, sizeof(*libItem));
    doc->lib.cameras = libItem;
  }

  cami = (void *)libItem->chld;
  if (cami) {
    cam->base.next = &cami->base;
  }

  libItem->chld = (void *)cam;
  /* Increment on every add — earlier this was set to 1 on library
     creation only, so adding a second / third camera left
     `count` at 1 and downstream code that sized arrays from this
     value silently truncated. */
  libItem->count++;

  return AK_OK;
}

AK_EXPORT
AkResult
ak_libAddLight(AkDoc   * __restrict doc,
               AkLight * __restrict light) {
  AkHeap    *heap;
  AkLibrary *libItem;
  AkLight   *lighti;

  heap    = ak_heap_getheap(doc);
  libItem = doc->lib.lights;
  if (!libItem) {
    libItem = ak_heap_calloc(heap, doc, sizeof(*libItem));
    doc->lib.lights = libItem;
  }

  lighti = (void *)libItem->chld;
  if (lighti) {
    light->next = lighti;
  }

  libItem->chld = (void *)light;
  libItem->count++;

  return AK_OK;
}

AK_EXPORT
void
ak_libInsertInto(AkLibrary *lib,
                 void      *item,
                 int32_t    prevOffset,
                 int32_t    nextOffset) {
  char *libChld, *lastLibChld;

  libChld = lastLibChld = (void *)lib->chld;
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
AkLibrary*
ak_libFirstOrCreat(AkDoc * __restrict doc,
                   uint32_t           itemOffset) {
  AkHeap    *heap;
  AkLibrary *lib;

  heap = ak_heap_getheap(doc);
  lib  = *(void **)((char *)&doc->lib + itemOffset);
  if (lib)
    return lib;

  lib = ak_heap_calloc(heap,
                       doc,
                       sizeof(*lib));
  doc->lib.libimages = lib;
  return lib;
}

AK_EXPORT
AkLibrary*
ak_libImageFirstOrCreat(AkDoc * __restrict doc) {
  return ak_libFirstOrCreat(doc, offsetof(AkLibraries, libimages));
}
