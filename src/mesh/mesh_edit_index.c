/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../memory_common.h"

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
      AkBuffer   *posbuff;

      if (!prim->pos
          || !(posacc  = prim->pos->accessor)
          || !(posbuff = posacc->buffer))
        return NULL;

      count = posacc->bound * posacc->count;
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
