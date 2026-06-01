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

#include <assert.h>

extern const char* ak_mesh_edit_assert1;

AK_EXPORT
AkIndexArray*
ak_meshIndicesArrayFor(AkMesh          * __restrict mesh,
                       AkMeshPrimitive * __restrict prim,
                       bool                         creat) {
  AkHeap           *heap;
  AkObject         *meshobj;
  AkIndexArray     *indices;
  size_t            count;
  AkUInt            maxIndex;
  AkTypeId          componentType;
  uint32_t          stride;

  assert(mesh->edith && ak_mesh_edit_assert1);

  indices = prim->reserved3;
  if (!indices) {
    if (!creat)
      return prim->indices;

    stride = prim->indexStride ? prim->indexStride : 1;
    if (prim->indices) {
      count = prim->indices->count / stride;
    } else if (prim->indexAccessor) {
      count = prim->indexAccessor->count / stride;
    } else {
      AkAccessor *posacc;

      if (!prim->pos
          || !(posacc  = prim->pos->accessor)
          || !posacc->buffer)
        return NULL;

      count = posacc->componentCount * posacc->count;
    }

    meshobj = ak_objFrom(mesh);
    heap    = ak_heap_getheap(meshobj);
    maxIndex      = count > 0 && count <= (size_t)UINT32_MAX
                    ? (AkUInt)(count - 1)
                    : UINT32_MAX;
    componentType = ak_indexComponentTypeForMax(maxIndex);
    indices       = ak_indexArrayAlloc(heap, prim, count, componentType);
    prim->reserved3 = indices;
    return indices;
  }

  return indices;
}

AK_EXPORT
void
ak_moveIndices(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  AkInput         *input;

  /* fix indices */
  prim = mesh->primitive;
  while (prim) {
    AkIndexArray *indices;

    indices = ak_meshIndicesArrayFor(mesh, prim, false);
    /* same index buff */
    if (!indices || indices == prim->indices) {
      prim = prim->next;
      continue;
    }

    ak_free(prim->indices);

    prim->indices = indices;
    prim->reserved3 = NULL;
    prim->indexAccessor = NULL;

    /* mark primitive as single index */
    prim->indexStride = 1;

    /* make all offsets 0 */
    input = prim->input;
    while (input) {
      input->indexOffset = 0;
      input = input->next;
    }

    prim = prim->next;
  }
}
