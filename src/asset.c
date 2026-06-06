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

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static
AkTree**
ak_extraField(void * __restrict obj) {
  AkHeap *heap;

  if (!obj)
    return NULL;

  heap = ak_heap_getheap(obj);
  if (heap && ak_heap_data(heap) == obj)
    return &((AkDoc *)obj)->extra;

  switch (ak_typeid(obj)) {
    case AKT_NODE:
      return &((AkNode *)obj)->extra;
    case AKT_SCENE:
      return &((AkScene *)obj)->extra;
    case AKT_GEOMETRY:
      return &((AkGeometry *)obj)->extra;
    case AKT_MESH:
      return &((AkMesh *)obj)->extra;
    case AKT_MATERIAL:
      return &((AkMaterial *)obj)->extra;
    case AKT_CAMERA:
      return &((AkCamera *)obj)->extra;
    case AKT_LIGHT:
      return &((AkLight *)obj)->extra;
    case AKT_EFFECT:
      return &((AkEffect *)obj)->extra;
    case AKT_PROFILE:
      return &((AkProfile *)obj)->extra;
    case AKT_TECHNIQUE_FX:
      return &((AkTechniqueFx *)obj)->extra;
    case AKT_SAMPLER:
    case AKT_SAMPLER2D:
      return &((AkSampler *)obj)->extra;
    default:
      break;
  }

  return NULL;
}

AK_EXPORT
AkTree*
ak_extra(void * __restrict obj) {
  AkHeapNode *hnode;
  AkTree    **extra;

  if (!obj)
    return NULL;

  hnode = ak__alignof(obj);
  if ((extra = ak_heap_ext_get(hnode, AK_HEAP_NODE_FLAGS_EXTRA)) && *extra)
    return *extra;

  if ((extra = ak_extraField(obj)))
    return *extra;

  return NULL;
}

AK_EXPORT
void
ak_extra_set(void   * __restrict obj,
             AkTree * __restrict extra) {
  AkHeap     *heap;
  AkHeapNode *hnode;
  AkTree    **slot;
  AkTree    **field;

  if (!obj)
    return;

  heap  = ak_heap_getheap(obj);
  hnode = ak__alignof(obj);
  slot  = ak_heap_ext_add(heap, hnode, AK_HEAP_NODE_FLAGS_EXTRA);
  *slot = extra;

  if ((field = ak_extraField(obj)))
    *field = extra;
}

AK_EXPORT
void*
ak_getAssetInfo(void * __restrict obj,
                size_t itemOffset) {
  AkHeapNode *hnode;
  char      **ai;
  void      **found;

  assert(obj && itemOffset < sizeof(AkAssetInf));

  
  hnode = ak__alignof(obj);

  do {
    if ((ai = ak_heap_ext_get(hnode, AK_HEAP_NODE_FLAGS_INF))
        && (found = (void **)(*ai + itemOffset)))
      return *found;

    hnode = ak_heap_parent(hnode);
    if (!hnode)
      return NULL;
  } while (true);
}

AK_EXPORT
AkCoordSys*
ak_getCoordSys(void * __restrict obj) {
  /* TODO: return default coord sys if null */
  return ak_getAssetInfo(obj, offsetof(AkAssetInf, coordSys));
}

AK_EXPORT
bool
ak_hasCoordSys(void * __restrict obj) {
  AkHeapNode *hnode;
  char      **ai;
  void      **found;

  hnode = ak__alignof(obj);
  ai    = ak_heap_ext_get(hnode, AK_HEAP_NODE_FLAGS_INF);
  if (!ai)
    return false;

  found = (void **)(*ai + offsetof(AkAssetInf, coordSys));
  return found != NULL;
}
