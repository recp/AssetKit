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

#include "index.h"
#include "../../include/ak/trash.h"

#include <limits.h>
#include <assert.h>

extern const char* ak_mesh_edit_assert1;

AK_HIDE
bool
ak_primCollapseIdentityIndices(AkMeshPrimitive *prim) {
  AkUIntArray *indices;
  AkInput     *input;
  AkUInt      *items;
  uint32_t     st, vo;
  size_t       count, i, j, base;
  AkUInt       posidx;

  if (!prim
      || prim->indexStride <= 1
      || !(indices = prim->indices)
      || !(items = indices->items)
      || !prim->pos)
    return false;

  st = prim->indexStride;
  vo = prim->pos->offset;
  if (st == 0 || vo >= st || indices->count == 0 || indices->count % st != 0)
    return false;

  count = indices->count / st;
  for (i = 0; i < count; i++) {
    base   = i * st;
    posidx = items[base + vo];
    for (j = 0; j < st; j++) {
      if (items[base + j] != posidx)
        return false;
    }
  }

  for (i = 0; i < count; i++)
    items[i] = items[i * st + vo];

  indices->count    = count;
  prim->indexStride = 1;
  prim->pos->offset = 0;

  for (input = prim->input; input; input = input->next)
    input->offset = 0;

  return true;
}

AK_HIDE
AkResult
ak_movePositions(AkMesh          *mesh,
                 AkMeshPrimitive *prim,
                 AkDuplicator    *duplicator) {
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;
  AkSourceBuffState  *buffstate;
  AkAccessor         *acc, *newacc;
  AkUIntArray        *dupc, *dupcsum;
  AkBuffer           *oldbuff, *newbuff;
  char               *olditms, *newitms;
  size_t              vc, d, s, pno, poo, byteStride;
  uint32_t            i, j;

  if (!prim->pos
      || !(edith     = mesh->edith)
      || !(acc       = prim->pos->accessor)
      || !(oldbuff   = acc->buffer)
      || !(buffstate = rb_find(edith->buffers, prim->pos)))
    return AK_ERR;

  newbuff = buffstate->buff;
  srch    = ak_meshSourceEditHelper(mesh, prim->pos);
  newacc  = srch->source;

  if (!newacc)
    return AK_ERR;

  dupc       = duplicator->range->dupc;
  dupcsum    = duplicator->range->dupcsum;
  vc         = dupc->count;
  newitms    = newbuff->data;
  olditms    = oldbuff->data;
  byteStride = acc->byteStride;

  /* copy vert positions to new location */
  for (i = 0; i < vc; i++) {
    if ((poo = dupc->items[3 * i + 2]) == 0)
      continue;

    pno = dupc->items[3 * i];
    d   = dupc->items[3 * i + 1];
    s   = dupcsum->items[pno];

    for (j = 0; j <= d; j++) {
      memcpy(newitms + byteStride * (pno + j + s),
             olditms + byteStride * (poo - 1),
             byteStride);
    }
  }

  return AK_OK;
}

AK_HIDE
AkResult
ak_primFixIndices(AkMesh          *mesh,
                  AkMeshPrimitive *prim) {
  AkDuplicator *dupl;

  if (prim->indexStride == 1 || !prim->indices)
    return AK_OK;

  if (ak_primCollapseIdentityIndices(prim))
    return AK_OK;

  if (!(dupl = ak_meshDuplicatorForIndices(mesh, prim)))
    return AK_ERR;

  ak_meshFixIndexBuffer(mesh, prim, dupl);
  ak_meshReserveBuffers(mesh, prim, dupl->dupCount + dupl->bufCount);
  ak_movePositions(mesh, prim, dupl);

  return AK_OK;
}

AK_HIDE
AkResult
ak_meshFixIndicesDefault(AkMesh *mesh) {
  AkMeshPrimitive *prim;

  prim = mesh->primitive;
  while (prim) {
    ak_primFixIndices(mesh, prim);
    prim = prim->next;
  }

  return AK_OK;
}

AK_HIDE
AkResult
ak_meshFixIndices(AkMesh *mesh) {
  AkResult ret;

  ak_meshBeginEdit(mesh);

  /* currently only default option */
  ret = ak_meshFixIndicesDefault(mesh);

  ak_meshEndEdit(mesh);

  return ret;
}
