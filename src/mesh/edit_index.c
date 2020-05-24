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
AkUIntArray*
ak_meshIndicesArrayFor(AkMesh          * __restrict mesh,
                       AkMeshPrimitive * __restrict prim,
                       bool                         creat) {
  AkHeap           *heap;
  AkObject         *meshobj;
  AkMeshEditHelper *edith;
  AkUIntArray      *indices;
  size_t            count;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  if (!(edith->flags & AK_GEOM_EDIT_FLAG_INDICES)
      || !edith->indices) {
    edith->indices = rb_newtree_ptr();
    edith->flags  |= AK_GEOM_EDIT_FLAG_INDICES;
    ak_dsSetAllocator(heap->allocator, edith->indices->alc);
  }

  indices = rb_find(edith->indices, prim);
  if (!indices) {
    if (!creat)
      return prim->indices;

    if (prim->indices) {
      count = prim->indices->count / prim->indexStride;
    } else {
      AkAccessor *posacc;

      if (!prim->pos
          || !(posacc  = prim->pos->accessor)
          || !posacc->buffer)
        return NULL;

      count = posacc->componentCount * posacc->count;
    }

    indices = ak_heap_calloc(heap,
                             prim,
                             sizeof(*indices)
                             + sizeof(AkUInt) * count);
    indices->count = count;

    rb_insert(edith->indices, prim, indices);
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
    AkUIntArray *indices;

    indices = ak_meshIndicesArrayFor(mesh, prim, false);
    /* same index buff */
    if (!indices || indices == prim->indices) {
      prim = prim->next;
      continue;
    }

    ak_free(prim->indices);

    prim->indices = indices;

    /* mark primitive as single index */
    prim->indexStride = 1;

    /* make all offsets 0 */
    input = prim->input;
    while (input) {
      input->offset = 0;
      input = input->next;
    }

    prim = prim->next;
  }
}
