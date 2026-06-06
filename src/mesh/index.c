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
#include <string.h>

extern const char* ak_mesh_edit_assert1;

AK_HIDE
bool
ak_primCollapseIdentityIndices(AkMeshPrimitive *prim) {
  AkIndexArray *indices;
  AkInput      *input;
  uint32_t      st, vo;
  size_t        count, i, j, base;
  AkUInt        posidx, maxIndex;

  if (!prim
      || prim->indexStride <= 1
      || !(indices = prim->indices)
      || !prim->pos)
    return false;

  st = prim->indexStride;
  vo = prim->pos->indexOffset;
  if (st == 0 || vo >= st || indices->count == 0 || indices->count % st != 0)
    return false;

  count = indices->count / st;
  for (i = 0; i < count; i++) {
    base   = i * st;
    posidx = ak_indexArrayGet(indices, base + vo);
    for (j = 0; j < st; j++) {
      if (ak_indexArrayGet(indices, base + j) != posidx)
        return false;
    }
  }

  maxIndex = 0;
  switch (indices->componentType) {
    case AKT_UBYTE: {
      uint8_t *items;

      items = (uint8_t *)indices->items;
      for (i = 0; i < count; i++) {
        posidx   = items[i * st + vo];
        items[i] = (uint8_t)posidx;
        if (posidx > maxIndex)
          maxIndex = posidx;
      }
      break;
    }
    case AKT_USHORT: {
      uint16_t *items;

      items = (uint16_t *)(void *)indices->items;
      for (i = 0; i < count; i++) {
        posidx   = items[i * st + vo];
        items[i] = (uint16_t)posidx;
        if (posidx > maxIndex)
          maxIndex = posidx;
      }
      break;
    }
    case AKT_UINT: {
      uint32_t *items;

      items = (uint32_t *)(void *)indices->items;
      for (i = 0; i < count; i++) {
        posidx   = items[i * st + vo];
        items[i] = posidx;
        if (posidx > maxIndex)
          maxIndex = posidx;
      }
      break;
    }
    default:
      return false;
  }

  indices->count         = count;
  indices->max           = maxIndex;
  prim->indexStride      = 1;
  prim->pos->indexOffset = 0;

  for (input = prim->input; input; input = input->next)
    input->indexOffset = 0;

  prim->indexAccessor = NULL;

  return true;
}

AK_HIDE
AkResult
ak_movePositions(AkMesh          *mesh,
                 AkMeshPrimitive *prim,
                 AkDuplicator    *duplicator) {
  AkBufferEditState  *buffstate;
  AkAccessor         *acc, *newacc;
  AkIndexArray       *dupc, *dupcsum;
  AkBuffer           *oldbuff, *newbuff;
  char               *olditms, *newitms;
  size_t              vc, d, s, pno, poo, copySize, srcStride, dstStride;
  uint32_t            i, j;

  if (!prim->pos
      || !mesh->edith
      || !(acc       = prim->pos->accessor)
      || !(oldbuff   = acc->buffer)
      || !(buffstate = prim->pos->reserved))
    return AK_ERR;

  newbuff = buffstate->buff;
  if (!(newacc = buffstate->accessor))
    return AK_ERR;

  dupc       = duplicator->range->dupc;
  dupcsum    = duplicator->range->dupcsum;
  vc         = dupc->count / 3;
  newitms    = newbuff->data;
  olditms    = oldbuff->data;
  copySize  = acc->fillByteSize;
  srcStride = acc->byteStride ? acc->byteStride : copySize;
  dstStride = newacc->byteStride ? newacc->byteStride : copySize;

  /* copy vert positions to new location */
#define AK_MOVE_POSITIONS_FOR_TYPE(DUPTYPE, SUMTYPE)                       \
  do {                                                                      \
    const DUPTYPE *dupcItems_;                                               \
    const SUMTYPE *sumItems_;                                                \
                                                                            \
    dupcItems_ = (const DUPTYPE *)(const void *)dupc->items;                 \
    sumItems_  = (const SUMTYPE *)(const void *)dupcsum->items;              \
    for (i = 0; i < vc; i++) {                                               \
      if ((poo = dupcItems_[3 * i + 2]) == 0)                                \
        continue;                                                            \
                                                                            \
      pno = dupcItems_[3 * i];                                               \
      d   = dupcItems_[3 * i + 1];                                           \
      s   = sumItems_[pno];                                                  \
                                                                            \
      for (j = 0; j <= d; j++) {                                             \
        memcpy(newitms + dstStride * (pno + j + s),                          \
               olditms + srcStride * (poo - 1),                              \
               copySize);                                                    \
      }                                                                      \
    }                                                                        \
  } while (0)

#define AK_MOVE_POSITIONS_FOR_SUM_TYPE(DUPTYPE)                             \
  do {                                                                      \
    switch (dupcsum->componentType) {                                        \
      case AKT_UBYTE:                                                        \
        AK_MOVE_POSITIONS_FOR_TYPE(DUPTYPE, uint8_t);                        \
        break;                                                               \
      case AKT_USHORT:                                                       \
        AK_MOVE_POSITIONS_FOR_TYPE(DUPTYPE, uint16_t);                       \
        break;                                                               \
      case AKT_UINT:                                                         \
        AK_MOVE_POSITIONS_FOR_TYPE(DUPTYPE, AkUInt);                         \
        break;                                                               \
      default:                                                               \
        break;                                                               \
    }                                                                        \
  } while (0)

  switch (dupc->componentType) {
    case AKT_UBYTE:
      AK_MOVE_POSITIONS_FOR_SUM_TYPE(uint8_t);
      break;
    case AKT_USHORT:
      AK_MOVE_POSITIONS_FOR_SUM_TYPE(uint16_t);
      break;
    case AKT_UINT:
      AK_MOVE_POSITIONS_FOR_SUM_TYPE(AkUInt);
      break;
    default:
      break;
  }

#undef AK_MOVE_POSITIONS_FOR_SUM_TYPE
#undef AK_MOVE_POSITIONS_FOR_TYPE

  return AK_OK;
}

AK_HIDE
AkResult
ak_primFixIndicesRetainDuplicator(AkMesh          *mesh,
                                  AkMeshPrimitive *prim,
                                  bool             retainDuplicator) {
  AkDuplicator *dupl;

  if (prim->indexStride == 1
      || (!prim->indices && !prim->indexAccessor))
    return AK_OK;

  if (ak_primCollapseIdentityIndices(prim))
    return AK_OK;

  if (!prim->indices) {
    ak_meshPrimitiveMaterializeIndices(prim);
    if (!prim->indices)
      return AK_ERR;
  }

  if (!(dupl = ak_meshDuplicatorForIndicesRetained(mesh,
                                                   prim,
                                                   retainDuplicator)))
    return AK_ERR;

  ak_meshFixIndexBuffer(mesh, prim, dupl);

  ak_meshReserveBuffers(mesh, prim, dupl->dupCount + dupl->bufCount);

  ak_movePositions(mesh, prim, dupl);

  if (!retainDuplicator)
    ak_free(dupl);

  return AK_OK;
}

AK_HIDE
AkResult
ak_primFixIndices(AkMesh          *mesh,
                  AkMeshPrimitive *prim) {
  return ak_primFixIndicesRetainDuplicator(mesh, prim, true);
}

AK_HIDE
AkResult
ak_meshFixIndicesDefaultRetainDuplicators(AkMesh *mesh,
                                          bool    retainDuplicators) {
  AkMeshPrimitive *prim;

  prim = mesh->primitive;
  while (prim) {
    ak_primFixIndicesRetainDuplicator(mesh, prim, retainDuplicators);
    prim = prim->next;
  }

  return AK_OK;
}

AK_HIDE
AkResult
ak_meshFixIndicesDefault(AkMesh *mesh) {
  return ak_meshFixIndicesDefaultRetainDuplicators(mesh, true);
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
