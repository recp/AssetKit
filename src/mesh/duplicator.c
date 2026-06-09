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
#include "index.h"
#include "edit_common.h"
#include <limits.h>

#define AK_INDEX_LOCAL_HASH_CAP 4096

AK_INLINE
uint32_t
ak_indexTupleHash(const AkUInt * __restrict tuple,
                  uint32_t                  stride) {
  uint32_t hash, i;

  hash = 2166136261u;

#define AK_INDEX_HASH_STEP(IDX) \
  do {                          \
    hash ^= tuple[IDX];         \
    hash *= 16777619u;          \
  } while (0)

  switch (stride) {
    case 1:
      AK_INDEX_HASH_STEP(0);
      break;
    case 2:
      AK_INDEX_HASH_STEP(0);
      AK_INDEX_HASH_STEP(1);
      break;
    case 3:
      AK_INDEX_HASH_STEP(0);
      AK_INDEX_HASH_STEP(1);
      AK_INDEX_HASH_STEP(2);
      break;
    case 4:
      AK_INDEX_HASH_STEP(0);
      AK_INDEX_HASH_STEP(1);
      AK_INDEX_HASH_STEP(2);
      AK_INDEX_HASH_STEP(3);
      break;
    default:
      for (i = 0; i < stride; i++) {
        AK_INDEX_HASH_STEP(i);
      }
      break;
  }

#undef AK_INDEX_HASH_STEP

  return hash ? hash : 1u;
}

#define AK_INDEX_TUPLE_HASH_TYPED(NAME, TYPE)                                \
AK_INLINE                                                                    \
uint32_t                                                                     \
NAME(const TYPE * __restrict tuple,                                          \
     uint32_t                  stride) {                                     \
  uint32_t hash, i;                                                          \
                                                                             \
  hash = 2166136261u;                                                        \
                                                                             \
  switch (stride) {                                                          \
    case 1:                                                                  \
      hash ^= tuple[0];                                                      \
      hash *= 16777619u;                                                     \
      break;                                                                 \
    case 2:                                                                  \
      hash ^= tuple[0];                                                      \
      hash *= 16777619u;                                                     \
      hash ^= tuple[1];                                                      \
      hash *= 16777619u;                                                     \
      break;                                                                 \
    case 3:                                                                  \
      hash ^= tuple[0];                                                      \
      hash *= 16777619u;                                                     \
      hash ^= tuple[1];                                                      \
      hash *= 16777619u;                                                     \
      hash ^= tuple[2];                                                      \
      hash *= 16777619u;                                                     \
      break;                                                                 \
    case 4:                                                                  \
      hash ^= tuple[0];                                                      \
      hash *= 16777619u;                                                     \
      hash ^= tuple[1];                                                      \
      hash *= 16777619u;                                                     \
      hash ^= tuple[2];                                                      \
      hash *= 16777619u;                                                     \
      hash ^= tuple[3];                                                      \
      hash *= 16777619u;                                                     \
      break;                                                                 \
    default:                                                                 \
      for (i = 0; i < stride; i++) {                                         \
        hash ^= tuple[i];                                                    \
        hash *= 16777619u;                                                   \
      }                                                                      \
      break;                                                                 \
  }                                                                          \
                                                                             \
  return hash ? hash : 1u;                                                   \
}

AK_INDEX_TUPLE_HASH_TYPED(ak_indexTupleHash8, uint8_t)
AK_INDEX_TUPLE_HASH_TYPED(ak_indexTupleHash16, uint16_t)

#undef AK_INDEX_TUPLE_HASH_TYPED

AK_INLINE
bool
ak_indexTupleEq(const AkUInt * __restrict a,
                const AkUInt * __restrict b,
                uint32_t                  stride) {
  switch (stride) {
    case 1:
      return a[0] == b[0];
    case 2:
      return a[0] == b[0] && a[1] == b[1];
    case 3:
      return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
    case 4:
      return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
    default:
      return memcmp(a, b, sizeof(*a) * stride) == 0;
  }
}

#define AK_INDEX_TUPLE_EQ_TYPED(NAME, TYPE)                                  \
AK_INLINE                                                                    \
bool                                                                         \
NAME(const TYPE * __restrict a,                                              \
     const TYPE * __restrict b,                                              \
     uint32_t                  stride) {                                     \
  switch (stride) {                                                          \
    case 1:                                                                  \
      return a[0] == b[0];                                                   \
    case 2:                                                                  \
      return a[0] == b[0] && a[1] == b[1];                                  \
    case 3:                                                                  \
      return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];                  \
    case 4:                                                                  \
      return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];  \
    default:                                                                 \
      return memcmp(a, b, sizeof(*a) * stride) == 0;                         \
  }                                                                          \
}

AK_INDEX_TUPLE_EQ_TYPED(ak_indexTupleEq8, uint8_t)
AK_INDEX_TUPLE_EQ_TYPED(ak_indexTupleEq16, uint16_t)

#undef AK_INDEX_TUPLE_EQ_TYPED

static
size_t
ak_indexHashCap(size_t count) {
  size_t cap, need;

  need = count < (SIZE_MAX / 2) ? count * 2 : count;
  cap  = 16;

  while (cap < need && cap < (SIZE_MAX / 2))
    cap <<= 1;

  return cap;
}

static
AkIndexArray*
ak_indexSideArrayAllocZero(AkHeap   * __restrict heap,
                           void     * __restrict parent,
                           size_t                count,
                           AkTypeId              componentType) {
  AkIndexArray *indices;
  size_t        itemSize;

  indices = ak_indexArrayAlloc(heap, parent, count, componentType);
  if (!indices)
    return NULL;

  itemSize = ak_indexComponentSize(componentType);
  memset(indices->items, 0, itemSize * count);

  return indices;
}

static
void
ak_meshFreeDuplicatorNode(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}

AK_HIDE
AkDuplicator*
ak_meshDuplicatorForIndicesRetained(AkMesh          * __restrict mesh,
                                    AkMeshPrimitive * __restrict prim,
                                    bool                         retain) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkObject           *meshobj;
  AkDuplicator       *dupl;
  AkDuplicatorRange  *dupr;
  AkIndexArray       *ind, *newind;
  AkIndexArray       *dupc, *dupcsum;
  uint32_t           *hashes;
  uint32_t            localHashes[AK_INDEX_LOCAL_HASH_CAP];
  AkAccessor         *posAcc;
  size_t              count, hcap, hmask, hpos, hslot, icount,
                      vertc, i, j, dupcItemCount, dupcsumCount;
  size_t              hashBytes;
  uint32_t            st, vo, posno, idxp, ord;
  AkUInt              dupcMax, dupcActualMax;
  AkTypeId            dupcType, dupcCompactType, dupcsumType;

  if (!prim->pos || !(posAcc = prim->pos->accessor))
    return NULL;

  vertc   = posAcc->count;
  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  doc     = ak_heap_data(heap);

  if (retain) {
    if (!doc->reserved) {
      doc->reserved = rb_newtree_ptr();
      ((RBTree *)doc->reserved)->onFreeNode = ak_meshFreeDuplicatorNode;
    }

    if (doc->reserved) {
      if ((dupl = rb_find(doc->reserved, prim))) {
        rb_remove(doc->reserved, prim);
        ak_free(dupl); /* or cache maybe if mesh is not edited ? */
      }
    }
  }

  dupl = ak_heap_calloc(heap, NULL, sizeof(*dupl));

  st   = prim->indexStride ? prim->indexStride : 1;
  vo   = prim->pos->indexOffset;
  ind  = prim->indices;

  if (!ind)
    goto fail;

  icount  = (uint32_t)(ind->count / st);
  newind  = ak_meshIndicesArrayFor(mesh, prim, true);

  if (!newind)
    goto fail;

  dupcMax       = (AkUInt)(vertc > icount ? vertc : icount);
  dupcActualMax = (AkUInt)vertc;
  dupcType      = ak_indexComponentTypeForMax(dupcMax);
  dupcItemCount = vertc * 3;

  /* TODO: cache this for multiple primitives */
  dupc = ak_indexSideArrayAllocZero(heap, dupl, dupcItemCount, dupcType);
  if (!dupc)
    goto fail;
  dupc->max = dupcMax;

  hcap   = ak_indexHashCap(icount);
  hmask  = hcap - 1;
  hashBytes = sizeof(uint32_t) * hcap;
  if (hcap <= AK_INDEX_LOCAL_HASH_CAP) {
    hashes = localHashes;
    memset(hashes, 0, hashBytes);
  } else {
    hashes = ak_heap_calloc(heap, dupl, hashBytes);
  }
  if (!hashes)
    goto fail;

  count = posno = 0;

#define AK_DUPLICATOR_FOR_INDEX_TYPE(DSTTYPE, TYPE, SRC, HASHFN, EQFN, SIDETYPE) \
  do {                                                                       \
    DSTTYPE    *dst_;                                                        \
    const TYPE *src_;                                                        \
    SIDETYPE   *dupcItems_;                                                  \
                                                                             \
    dst_ = (DSTTYPE *)(void *)newind->items;                                 \
    src_ = (const TYPE *)(const void *)(SRC);                                \
    dupcItems_ = (SIDETYPE *)(void *)dupc->items;                            \
    for (j = i = 0; j < icount; j++, i += st) {                              \
      idxp = (uint32_t)src_[i + vo];                                         \
      if (idxp >= vertc)                                                     \
        goto fail;                                                           \
                                                                             \
      hpos = HASHFN(&src_[i], st) & hmask;                                   \
      for (;;) {                                                             \
        hslot = hashes[hpos];                                                \
        if (!hslot) {                                                        \
          if (dupcItems_[3 * idxp + 2] == 0) {                               \
            dupcItems_[3 * idxp]     = (SIDETYPE)posno++;                    \
            dupcItems_[3 * idxp + 2] = (SIDETYPE)(idxp + 1);                 \
            ord = 0;                                                         \
          } else {                                                           \
            ord = ++dupcItems_[3 * idxp + 1];                                \
            if ((AkUInt)ord > dupcActualMax)                                 \
              dupcActualMax = (AkUInt)ord;                                   \
            count++;                                                         \
          }                                                                  \
                                                                             \
          hashes[hpos] = (uint32_t)j + 1;                                    \
          dst_[j]      = (DSTTYPE)ord;                                       \
          break;                                                             \
        }                                                                    \
                                                                             \
        hslot--;                                                             \
        if (EQFN(&src_[i], &src_[hslot * st], st)) {                         \
          dst_[j] = dst_[hslot];                                             \
          break;                                                             \
        }                                                                    \
                                                                             \
        hpos = (hpos + 1) & hmask;                                           \
      }                                                                      \
    }                                                                        \
  } while (0)

#define AK_DUPLICATOR_FOR_OUTPUT_TYPE(DSTTYPE, SIDETYPE)                     \
  do {                                                                       \
    switch (ind->componentType) {                                            \
      case AKT_UBYTE:                                                        \
        AK_DUPLICATOR_FOR_INDEX_TYPE(DSTTYPE,                                \
                                     uint8_t,                                \
                                     ind->items,                             \
                                     ak_indexTupleHash8,                     \
                                     ak_indexTupleEq8,                       \
                                     SIDETYPE);                              \
        break;                                                               \
      case AKT_USHORT:                                                       \
        AK_DUPLICATOR_FOR_INDEX_TYPE(DSTTYPE,                                \
                                     uint16_t,                               \
                                     ind->items,                             \
                                     ak_indexTupleHash16,                    \
                                     ak_indexTupleEq16,                      \
                                     SIDETYPE);                              \
        break;                                                               \
      case AKT_UINT:                                                         \
        AK_DUPLICATOR_FOR_INDEX_TYPE(DSTTYPE,                                \
                                     AkUInt,                                 \
                                     ind->items,                             \
                                     ak_indexTupleHash,                      \
                                     ak_indexTupleEq,                        \
                                     SIDETYPE);                              \
        break;                                                               \
      default:                                                               \
        goto fail;                                                           \
    }                                                                        \
  } while (0)

#define AK_DUPLICATOR_FOR_SIDE_TYPE(SIDETYPE)                                \
  do {                                                                       \
    switch (newind->componentType) {                                         \
      case AKT_UBYTE:                                                        \
        AK_DUPLICATOR_FOR_OUTPUT_TYPE(uint8_t, SIDETYPE);                    \
        break;                                                               \
      case AKT_USHORT:                                                       \
        AK_DUPLICATOR_FOR_OUTPUT_TYPE(uint16_t, SIDETYPE);                   \
        break;                                                               \
      case AKT_UINT:                                                         \
        AK_DUPLICATOR_FOR_OUTPUT_TYPE(AkUInt, SIDETYPE);                     \
        break;                                                               \
      default:                                                               \
        goto fail;                                                           \
    }                                                                        \
  } while (0)

  switch (dupc->componentType) {
    case AKT_UBYTE:
      AK_DUPLICATOR_FOR_SIDE_TYPE(uint8_t);
      break;
    case AKT_USHORT:
      AK_DUPLICATOR_FOR_SIDE_TYPE(uint16_t);
      break;
    case AKT_UINT:
      AK_DUPLICATOR_FOR_SIDE_TYPE(AkUInt);
      break;
    default:
      goto fail;
  }

#undef AK_DUPLICATOR_FOR_SIDE_TYPE
#undef AK_DUPLICATOR_FOR_OUTPUT_TYPE
#undef AK_DUPLICATOR_FOR_INDEX_TYPE

  dupc->max       = dupcActualMax;
  dupcCompactType = ak_indexComponentTypeForMax(dupcActualMax);
  if (ak_indexComponentSize(dupcCompactType)
      < ak_indexComponentSize(dupc->componentType))
    dupc = ak_indexArrayPromote(heap, dupl, dupc, dupcCompactType);

  dupcsumType  = ak_indexComponentTypeForMax((AkUInt)count);
  dupcsumCount = (size_t)posno + 1;
  dupcsum      = ak_indexSideArrayAllocZero(heap, dupc, dupcsumCount, dupcsumType);
  if (!dupcsum)
    goto fail;
  dupcsum->max = (AkUInt)count;

#define AK_BUILD_DUPCSUM(DUPTYPE, SUMTYPE)                                  \
  do {                                                                      \
    const DUPTYPE *dupcItems_;                                               \
    SUMTYPE       *sumItems_;                                                \
    AkUInt         pno_, d_, sum_;                                           \
                                                                            \
    dupcItems_ = (const DUPTYPE *)(const void *)dupc->items;                 \
    sumItems_  = (SUMTYPE *)(void *)dupcsum->items;                          \
                                                                            \
    for (i = 0; i < vertc; i++) {                                            \
      if (dupcItems_[3 * i + 2] == 0)                                        \
        continue;                                                            \
                                                                            \
      pno_ = (AkUInt)dupcItems_[i * 3];                                      \
      d_   = (AkUInt)dupcItems_[i * 3 + 1];                                  \
                                                                            \
      sumItems_[pno_ + 1] = (SUMTYPE)d_;                                     \
    }                                                                        \
                                                                            \
    for (i = 1; i < dupcsum->count; i++) {                                   \
      sum_        = (AkUInt)sumItems_[i] + (AkUInt)sumItems_[i - 1];          \
      sumItems_[i] = (SUMTYPE)sum_;                                          \
    }                                                                        \
  } while (0)

#define AK_BUILD_DUPCSUM_FOR_DUP_TYPE(DUPTYPE)                              \
  do {                                                                      \
    switch (dupcsum->componentType) {                                        \
      case AKT_UBYTE:                                                        \
        AK_BUILD_DUPCSUM(DUPTYPE, uint8_t);                                  \
        break;                                                               \
      case AKT_USHORT:                                                       \
        AK_BUILD_DUPCSUM(DUPTYPE, uint16_t);                                 \
        break;                                                               \
      case AKT_UINT:                                                         \
        AK_BUILD_DUPCSUM(DUPTYPE, AkUInt);                                   \
        break;                                                               \
      default:                                                               \
        goto fail;                                                           \
    }                                                                        \
  } while (0)

  switch (dupc->componentType) {
    case AKT_UBYTE:
      AK_BUILD_DUPCSUM_FOR_DUP_TYPE(uint8_t);
      break;
    case AKT_USHORT:
      AK_BUILD_DUPCSUM_FOR_DUP_TYPE(uint16_t);
      break;
    case AKT_UINT:
      AK_BUILD_DUPCSUM_FOR_DUP_TYPE(AkUInt);
      break;
    default:
      goto fail;
  }

#undef AK_BUILD_DUPCSUM_FOR_DUP_TYPE
#undef AK_BUILD_DUPCSUM

  dupr             = ak_heap_alloc(heap, dupl, sizeof(*dupr));
  dupr->dupc       = dupc;
  dupr->startIndex = 0;
  dupr->endIndex   = posAcc->count;
  dupr->next       = NULL;
  dupr->dupcsum    = dupcsum;

  dupl->range      = dupr;
  dupl->dupCount   = count;
  dupl->bufCount   = posno;

  if (hashes != localHashes)
    ak_free(hashes);

  if (retain && doc->reserved)
    rb_insert(doc->reserved, prim, dupl);

  return dupl;

fail:
  ak_free(dupl);
  return NULL;
}

AK_EXPORT
AkDuplicator*
ak_meshDuplicatorForIndices(AkMesh          * __restrict mesh,
                            AkMeshPrimitive * __restrict prim) {
  return ak_meshDuplicatorForIndicesRetained(mesh, prim, true);
}

AK_EXPORT
void
ak_meshFixIndexBuffer(AkMesh          * __restrict mesh,
                      AkMeshPrimitive * __restrict prim,
                      AkDuplicator    * __restrict duplicator) {
  AkDuplicatorRange *dupr;
  AkIndexArray      *ind, *newind;
  AkIndexArray      *dupc, *dupcsum;
  uint32_t           i, j, c, st, vo, idxp, nidxp;

  dupr    = duplicator->range;
  dupc    = dupr->dupc;
  dupcsum = dupr->dupcsum;

  newind = ak_meshIndicesArrayFor(mesh, prim, true);
  ind    = prim->indices;
  if (!newind || !ind)
    return;

  st     = prim->indexStride ? prim->indexStride : 1;
  vo     = prim->pos->indexOffset;
  c      = (uint32_t)ind->count;

#define AK_FIX_INDEX_BUFFER_FOR_TYPE(DSTTYPE, TYPE, SRC, DUPTYPE, SUMTYPE)   \
  do {                                                                       \
    DSTTYPE    *dst_;                                                        \
    const TYPE *src_;                                                        \
    const DUPTYPE *dupcItems_;                                               \
    const SUMTYPE *sumItems_;                                                \
                                                                             \
    dst_ = (DSTTYPE *)(void *)newind->items;                                 \
    src_ = (const TYPE *)(const void *)(SRC);                                \
    dupcItems_ = (const DUPTYPE *)(const void *)dupc->items;                 \
    sumItems_  = (const SUMTYPE *)(const void *)dupcsum->items;              \
    if (duplicator->dupCount > 0) {                                          \
      for (i = j = 0; i < c; i += st, j++) {                                 \
        idxp   = (uint32_t)src_[i + vo];                                     \
        nidxp  = (uint32_t)dupcItems_[idxp * 3];                             \
        dst_[j] = (DSTTYPE)(dst_[j] + nidxp + sumItems_[nidxp]);             \
      }                                                                      \
    } else {                                                                 \
      for (i = j = 0; i < c; i += st, j++) {                                 \
        idxp   = (uint32_t)src_[i + vo];                                     \
        nidxp  = (uint32_t)dupcItems_[idxp * 3];                             \
        dst_[j] = (DSTTYPE)nidxp;                                            \
      }                                                                      \
    }                                                                        \
  } while (0)

#define AK_FIX_INDEX_BUFFER_FOR_OUTPUT_TYPE(DSTTYPE, DUPTYPE, SUMTYPE)       \
  do {                                                                       \
    switch (ind->componentType) {                                            \
      case AKT_UBYTE:                                                        \
        AK_FIX_INDEX_BUFFER_FOR_TYPE(DSTTYPE, uint8_t, ind->items, DUPTYPE, SUMTYPE); \
        break;                                                               \
      case AKT_USHORT:                                                       \
        AK_FIX_INDEX_BUFFER_FOR_TYPE(DSTTYPE, uint16_t, ind->items, DUPTYPE, SUMTYPE); \
        break;                                                               \
      case AKT_UINT:                                                         \
        AK_FIX_INDEX_BUFFER_FOR_TYPE(DSTTYPE, AkUInt, ind->items, DUPTYPE, SUMTYPE); \
        break;                                                               \
      default:                                                               \
        break;                                                               \
    }                                                                        \
  } while (0)

#define AK_FIX_INDEX_BUFFER_FOR_SUM_TYPE(DUPTYPE, SUMTYPE)                  \
  do {                                                                      \
    switch (newind->componentType) {                                        \
      case AKT_UBYTE:                                                       \
        AK_FIX_INDEX_BUFFER_FOR_OUTPUT_TYPE(uint8_t, DUPTYPE, SUMTYPE);     \
        break;                                                              \
      case AKT_USHORT:                                                      \
        AK_FIX_INDEX_BUFFER_FOR_OUTPUT_TYPE(uint16_t, DUPTYPE, SUMTYPE);    \
        break;                                                              \
      case AKT_UINT:                                                        \
        AK_FIX_INDEX_BUFFER_FOR_OUTPUT_TYPE(AkUInt, DUPTYPE, SUMTYPE);      \
        break;                                                              \
      default:                                                              \
        break;                                                              \
    }                                                                       \
  } while (0)

#define AK_FIX_INDEX_BUFFER_FOR_DUP_TYPE(DUPTYPE)                           \
  do {                                                                      \
    switch (dupcsum->componentType) {                                       \
      case AKT_UBYTE:                                                       \
        AK_FIX_INDEX_BUFFER_FOR_SUM_TYPE(DUPTYPE, uint8_t);                 \
        break;                                                              \
      case AKT_USHORT:                                                      \
        AK_FIX_INDEX_BUFFER_FOR_SUM_TYPE(DUPTYPE, uint16_t);                \
        break;                                                              \
      case AKT_UINT:                                                        \
        AK_FIX_INDEX_BUFFER_FOR_SUM_TYPE(DUPTYPE, AkUInt);                  \
        break;                                                              \
      default:                                                              \
        break;                                                              \
    }                                                                       \
  } while (0)

  switch (dupc->componentType) {
    case AKT_UBYTE:
      AK_FIX_INDEX_BUFFER_FOR_DUP_TYPE(uint8_t);
      break;
    case AKT_USHORT:
      AK_FIX_INDEX_BUFFER_FOR_DUP_TYPE(uint16_t);
      break;
    case AKT_UINT:
      AK_FIX_INDEX_BUFFER_FOR_DUP_TYPE(AkUInt);
      break;
    default:
      break;
  }

  newind->max = ak_indicesMax(newind);

#undef AK_FIX_INDEX_BUFFER_FOR_DUP_TYPE
#undef AK_FIX_INDEX_BUFFER_FOR_SUM_TYPE
#undef AK_FIX_INDEX_BUFFER_FOR_OUTPUT_TYPE
#undef AK_FIX_INDEX_BUFFER_FOR_TYPE
}
