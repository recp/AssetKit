/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "mesh_index.h"
#include "../../include/ak/trash.h"
#include "mesh_util.h"

#include <limits.h>
#include <assert.h>

extern const char* ak_mesh_edit_assert1;

_assetkit_hide
AkResult
ak_movePositions(AkHeap          *heap,
                 AkMesh          *mesh,
                 AkMeshPrimitive *prim,
                 AkDuplicator    *duplicator) {
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;
  AkSourceBuffState  *buffstate;
  AkSource           *src;
  AkAccessor         *acc, *newacc;
  AkDataParam        *dparam;
  AkUIntArray        *dupc, *dupcsum;
  AkBuffer           *oldbuff, *newbuff;
  AkFloat            *olditms, *newitms;
  size_t              vc, d, s, pno, poo;
  uint32_t            stride, i, j;

  if (!prim->pos
      || !(edith     = mesh->edith)
      || !(src       = ak_getObjectByUrl(&prim->pos->source))
      || !(acc       = src->tcommon)
      || !(oldbuff   = ak_getObjectByUrl(&acc->source))
      || !(buffstate = rb_find(edith->buffers, prim->pos)))
    return AK_ERR;

  newbuff = buffstate->buff;
  srch    = ak_meshSourceEditHelper(mesh, prim->pos);
  newacc  = srch->source->tcommon;

  if (!newacc)
    return AK_ERR;

  newacc->firstBound = buffstate->lastoffset;
  ak_accessor_rebound(heap,
                      newacc,
                      buffstate->lastoffset);

  dupc    = duplicator->range->dupc;
  dupcsum = duplicator->range->dupcsum;
  vc      = dupc->count;
  stride  = acc->stride;
  newitms = newbuff->data;
  olditms = oldbuff->data;

  /* copy vert positions to new location */
  for (i = 0; i < vc; i++) {
    if ((poo = dupc->items[3 * i + 2]) == 0)
      continue;

    pno = dupc->items[3 * i];
    d   = dupc->items[3 * i + 1];
    s   = dupcsum->items[pno];

    for (j = 0; j <= d; j++) {
      dparam = acc->param;

      while (dparam) {
        if (!dparam->name)
          continue;

        /* in new design newstride is always 1 */
        newitms[(pno + j + s) * acc->bound + dparam->offset]
          = olditms[(poo - 1) * stride + dparam->offset];

        dparam = dparam->next;
      }
    }
  }

  return AK_OK;
}

_assetkit_hide
AkResult
ak_primFixIndices(AkHeap          *heap,
                  AkMesh          *mesh,
                  AkMeshPrimitive *prim) {
  AkDuplicator *dupl;

  if (!prim->indices || !(dupl = ak_meshDuplicatorForIndices(mesh, prim)))
    return AK_ERR;

  ak_meshFixIndexBuffer(mesh, prim, dupl);
  ak_meshReserveBuffers(mesh, prim, dupl->dupCount + dupl->bufCount);
  ak_movePositions(heap, mesh, prim, dupl);

  return AK_OK;
}

_assetkit_hide
AkResult
ak_meshFixIndicesDefault(AkHeap *heap, AkMesh *mesh) {
  AkMeshPrimitive *prim;

  prim = mesh->primitive;
  while (prim) {
    ak_primFixIndices(heap, mesh, prim);
    prim = prim->next;
  }

  return AK_OK;
}

_assetkit_hide
AkResult
ak_meshFixIndices(AkHeap *heap, AkMesh *mesh) {
  AkResult ret;

  ak_meshBeginEdit(mesh);

  /* currently only default option */
  ret = ak_meshFixIndicesDefault(heap, mesh);

  ak_meshEndEdit(mesh);

  return ret;
}
