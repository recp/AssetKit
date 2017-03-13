/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "ak_mesh_util.h"

#include <assert.h>

extern const char* ak_mesh_edit_assert1;

AK_EXPORT
AkUIntArray*
ak_meshIndicesArrayFor(AkMesh          * __restrict mesh,
                       AkMeshPrimitive * __restrict prim) {
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
    edith->indices = rb_newtree(ak_cmp_ptr, NULL);
    edith->flags |= AK_GEOM_EDIT_FLAG_INDICES;
  }

  indices = rb_find(edith->indices, prim);
  if (!indices) {
    if (prim->indices) {
      count = prim->indices->count / prim->indexStride;
    } else {
      AkSource          *possrc;
      AkAccessor        *posacc;
      AkSourceArrayBase *posarr;
      AkObject          *posdata;

      possrc = ak_mesh_pos_src(mesh);
      if (!possrc || !possrc->tcommon)
        return NULL;

      posacc  = possrc->tcommon;
      posdata = ak_getObjectByUrl(&posacc->source);
      if (!posdata)
        return NULL;

      posarr = ak_objGet(posdata);
      count  = posarr->count;
    }

    indices = ak_heap_alloc(heap,
                            prim,
                            sizeof(*indices)
                            + sizeof(AkUInt) * count);
    indices->count = count;
    rb_insert(edith->indices, prim, indices);
    return indices;
  }

  return indices;
}
