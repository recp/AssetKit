/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../memory_common.h"
#include "mesh_util.h"
#include "mesh_edit_common.h"

const char*
ak_mesh_edit_assert1 = "you must call ak_meshBeginEdit* before this op";

AK_EXPORT
void
ak_meshBeginEdit(AkMesh * __restrict mesh) {
  ak_meshBeginEditA(mesh,
                    AK_GEOM_EDIT_FLAG_ARRAYS
                    | AK_GEOM_EDIT_FLAG_INDICES);
}

AK_EXPORT
void
ak_meshBeginEditA(AkMesh  * __restrict mesh,
                  AkGeometryEditFlags  flags) {
  AkHeap           *heap;
  AkObject         *meshobj;
  AkMeshEditHelper *edith;

  edith   = mesh->edith;
  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  if (!edith) {
    mesh->edith = edith = ak_heap_calloc(heap,
                                         ak_objFrom(mesh),
                                         sizeof(*edith));
    ak_heap_ext_add(heap,
                    ak__alignof(edith),
                    AK_HEAP_NODE_FLAGS_REFC);
  }

  if ((flags & AK_GEOM_EDIT_FLAG_ARRAYS)
      && !edith->buffers) {
    edith->buffers         = rb_newtree_ptr();
    edith->inputBufferMap  = ak_map_new(ak_cmp_ptr);

    ak_dsSetAllocator(heap->allocator, edith->buffers->alc);

    edith->buffers->onFreeNode = ak_meshFreeRsvBuff;

    edith->flags |= AK_GEOM_EDIT_FLAG_ARRAYS;
  }

  if ((flags & AK_GEOM_EDIT_FLAG_INDICES)
      && !edith->buffers) {
    edith->indices = rb_newtree_ptr();
    edith->flags  |= AK_GEOM_EDIT_FLAG_INDICES;
    ak_dsSetAllocator(heap->allocator, edith->indices->alc);
  }

  ak_retain(edith);
}

AK_EXPORT
void
ak_meshEndEdit(AkMesh * __restrict mesh) {
  AkMeshEditHelper *edith;

  edith = mesh->edith;
  if (!edith)
    return;

  /* finish edit */
  ak_moveIndices(mesh);
  ak_meshMoveBuffers(mesh);

  if (edith->buffers)
    rb_destroy(edith->buffers);

  if (edith->indices)
    rb_destroy(edith->indices);

  if (edith->inputBufferMap)
    ak_map_destroy(edith->inputBufferMap);

  ak_release(edith);
  mesh->edith = NULL;
}
