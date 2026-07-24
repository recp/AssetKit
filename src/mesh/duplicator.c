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
#define AK_INDEX_CHAIN_PROBE_LIMIT 32

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
AkDuplicator*
ak_meshDuplicatorForIndicesRetainedLegacy(AkMesh          * __restrict mesh,
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
                      vertc, i, j, dupcItemCount, dupcsumCount,
                      hashItemCount, chainItemCount;
  size_t              hashBytes, dupcBytes;
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
    if ((dupl = ak__docDuplicatorFind(doc, prim))) {
      ak_free(dupl); /* or cache maybe if mesh is not edited ? */
      (void)ak__docDuplicatorSet(doc, prim, NULL);
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

  hcap = ak_indexHashCap(icount);
  hmask = hcap - 1;
  if (vertc > SIZE_MAX - icount)
    goto fail;
  chainItemCount = vertc + icount;
  hashItemCount = hcap > chainItemCount ? hcap : chainItemCount;
  if (hashItemCount > SIZE_MAX / sizeof(uint32_t)
      || dupc->count > SIZE_MAX
                       / ak_indexComponentSize(dupc->componentType))
    goto fail;
  hashBytes     = sizeof(uint32_t) * hashItemCount;
  dupcBytes     = ak_indexComponentSize(dupc->componentType) * dupc->count;
  if (hashItemCount <= AK_INDEX_LOCAL_HASH_CAP) {
    hashes = localHashes;
    memset(hashes, 0, hashBytes);
  } else {
    hashes = ak_heap_calloc(heap, dupl, hashBytes);
  }
  if (!hashes)
    goto fail;

  count = posno = 0;

#define AK_DUPLICATOR_ASSIGN_NEW(DSTTYPE, SIDETYPE)                          \
  do {                                                                       \
    if (dupcItems_[3 * idxp + 2] == 0) {                                     \
      dupcItems_[3 * idxp]     = (SIDETYPE)posno++;                          \
      dupcItems_[3 * idxp + 2] = (SIDETYPE)(idxp + 1);                       \
      ord = 0;                                                               \
    } else {                                                                 \
      ord = ++dupcItems_[3 * idxp + 1];                                      \
      if ((AkUInt)ord > dupcActualMax)                                       \
        dupcActualMax = (AkUInt)ord;                                         \
      count++;                                                               \
    }                                                                        \
                                                                             \
    dst_[j] = (DSTTYPE)ord;                                                  \
  } while (0)

#define AK_DUPLICATOR_FOR_INDEX_TYPE(DSTTYPE, TYPE, SRC, HASHFN, EQFN, SIDETYPE) \
  do {                                                                       \
    DSTTYPE    *dst_;                                                        \
    const TYPE *src_;                                                        \
    SIDETYPE   *dupcItems_;                                                  \
    size_t      chainProbe_;                                                 \
    bool        chainFallback_, found_;                                      \
                                                                             \
    dst_ = (DSTTYPE *)(void *)newind->items;                                 \
    src_ = (const TYPE *)(const void *)(SRC);                                \
    dupcItems_ = (SIDETYPE *)(void *)dupc->items;                            \
    chainFallback_ = false;                                                  \
    for (j = i = 0; j < icount; j++, i += st) {                              \
      idxp = (uint32_t)src_[i + vo];                                         \
      if (idxp >= vertc)                                                     \
        goto fail;                                                           \
                                                                             \
      found_      = false;                                                   \
      chainProbe_ = 0;                                                       \
      hslot       = hashes[idxp];                                            \
      while (hslot) {                                                        \
        hslot--;                                                             \
        if (EQFN(&src_[i], &src_[hslot * st], st)) {                         \
          dst_[j] = dst_[hslot];                                             \
          found_  = true;                                                    \
          break;                                                             \
        }                                                                    \
                                                                             \
        if (++chainProbe_ >= AK_INDEX_CHAIN_PROBE_LIMIT) {                   \
          chainFallback_ = true;                                             \
          break;                                                             \
        }                                                                    \
                                                                             \
        hslot = hashes[vertc + hslot];                                       \
      }                                                                      \
      if (chainFallback_)                                                    \
        break;                                                               \
      if (found_)                                                            \
        continue;                                                            \
                                                                             \
      hashes[vertc + j] = hashes[idxp];                                      \
      hashes[idxp]       = (uint32_t)j + 1;                                  \
      AK_DUPLICATOR_ASSIGN_NEW(DSTTYPE, SIDETYPE);                           \
    }                                                                        \
                                                                             \
    if (chainFallback_) {                                                    \
      memset(dupcItems_, 0, dupcBytes);                                      \
      memset(hashes, 0, hashBytes);                                          \
      count = posno = 0;                                                     \
      dupcActualMax = (AkUInt)vertc;                                         \
                                                                             \
      for (j = i = 0; j < icount; j++, i += st) {                            \
        idxp = (uint32_t)src_[i + vo];                                       \
        hpos = HASHFN(&src_[i], st) & hmask;                                 \
        for (;;) {                                                           \
          hslot = hashes[hpos];                                              \
          if (!hslot) {                                                      \
            hashes[hpos] = (uint32_t)j + 1;                                  \
            AK_DUPLICATOR_ASSIGN_NEW(DSTTYPE, SIDETYPE);                     \
            break;                                                           \
          }                                                                  \
                                                                             \
          hslot--;                                                           \
          if (EQFN(&src_[i], &src_[hslot * st], st)) {                       \
            dst_[j] = dst_[hslot];                                           \
            break;                                                           \
          }                                                                  \
                                                                             \
          hpos = (hpos + 1) & hmask;                                         \
        }                                                                    \
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
#undef AK_DUPLICATOR_ASSIGN_NEW

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

  if (retain && !ak__docDuplicatorSet(doc, prim, dupl))
    goto fail;

  return dupl;

fail:
  ak_free(dupl);
  return NULL;
}

AK_HIDE
bool
ak_meshDuplicatorBuildPrepare(AkMeshPrimitive   * __restrict prim,
                              AkDuplicatorBuild * __restrict build) {
  AkAccessor   *posAcc;
  AkIndexArray *indices;
  size_t        indexCount, vertexCount, hashCount, chainCount, lookupCount,
                totalItems;
  uint32_t      stride, positionOffset;

  if (!prim || !build)
    return false;

  memset(build, 0, sizeof(*build));

  if (!prim->pos
      || !(posAcc = prim->pos->accessor)
      || !(indices = prim->indices))
    return false;

  stride         = prim->indexStride ? prim->indexStride : 1;
  positionOffset = prim->pos->indexOffset;
  if (positionOffset >= stride || indices->count % stride != 0)
    return false;

  indexCount  = indices->count / stride;
  vertexCount = posAcc->count;
  if (indexCount == 0
      || indexCount > UINT32_MAX
      || vertexCount > UINT32_MAX
      || vertexCount > SIZE_MAX - indexCount)
    return false;

  hashCount  = ak_indexHashCap(indexCount);
  chainCount = vertexCount + indexCount;
  lookupCount = hashCount > chainCount ? hashCount : chainCount;
  if (vertexCount > SIZE_MAX / 3
      || indexCount > SIZE_MAX - vertexCount * 3
      || lookupCount > SIZE_MAX - indexCount - vertexCount * 3)
    return false;
  totalItems = indexCount + vertexCount * 3 + lookupCount;
  if (totalItems > SIZE_MAX / sizeof(uint32_t))
    return false;

  build->primitive        = prim;
  build->indices          = indices;
  build->positionAccessor = posAcc;
  build->indexCount       = indexCount;
  build->vertexCount      = vertexCount;
  build->lookupCount      = lookupCount;
  build->storageSize      = totalItems * sizeof(uint32_t);
  build->indexStride      = stride;
  build->positionOffset   = positionOffset;
  return true;
}

AK_HIDE
void
ak_meshDuplicatorBuildCompute(void *userdata) {
  AkDuplicatorBuild *build;
  const AkIndexArray *indices;
  uint32_t          *ordinals, *positionCopies, *lookup;
  size_t             totalItems, storageBytes, hashCount, hashMask;
  size_t             i, j, slot, hashPosition, chainProbe;
  uint32_t           positionIndex, ordinal, positionNumber;
  AkUInt             duplicateMax;
  size_t             duplicateCount;
  bool               chainFallback, found;

  build = userdata;
  if (!build || build->computed)
    return;

  build->computed = true;
  build->valid    = false;

  totalItems   = build->indexCount
                 + build->vertexCount * 3
                 + build->lookupCount;
  storageBytes = totalItems * sizeof(uint32_t);
  if (!build->storage) {
    build->storage = malloc(storageBytes);
    if (!build->storage)
      return;
    build->ownsStorage = true;
  }

  ordinals       = build->storage;
  positionCopies = ordinals + build->indexCount;
  lookup         = positionCopies + build->vertexCount * 3;
  memset(positionCopies,
         0,
         (build->vertexCount * 3 + build->lookupCount) * sizeof(uint32_t));

  build->ordinals       = ordinals;
  build->positionCopies = positionCopies;
  build->lookup         = lookup;

  indices       = build->indices;
  hashCount     = ak_indexHashCap(build->indexCount);
  hashMask      = hashCount - 1;
  positionNumber = 0;
  duplicateCount = 0;
  duplicateMax   = (AkUInt)build->vertexCount;
  chainFallback  = false;

  /*
   * lookup starts with one head per position followed by one link per tuple.
   * Most authored meshes have only a few UV/normal variants per position, so
   * these short chains avoid the random probes of a global hash table. Fall
   * back to the bounded open-addressing path for pathological long chains.
   */
#define AK_DUPLICATOR_BUILD_ASSIGN_NEW()                                    \
  do {                                                                       \
    if (positionCopies[3 * positionIndex + 2] == 0) {                        \
      positionCopies[3 * positionIndex]     = positionNumber++;             \
      positionCopies[3 * positionIndex + 2] = positionIndex + 1;            \
      ordinal = 0;                                                           \
    } else {                                                                 \
      ordinal = ++positionCopies[3 * positionIndex + 1];                     \
      if ((AkUInt)ordinal > duplicateMax)                                    \
        duplicateMax = (AkUInt)ordinal;                                      \
      duplicateCount++;                                                      \
    }                                                                        \
                                                                             \
    ordinals[j] = ordinal;                                                   \
  } while (0)

#define AK_DUPLICATOR_BUILD_FOR_TYPE(TYPE, HASHFN, EQFN)                    \
  do {                                                                       \
    const TYPE *src_;                                                        \
                                                                             \
    src_ = (const TYPE *)(const void *)indices->items;                       \
    for (j = i = 0; j < build->indexCount;                                  \
         j++, i += build->indexStride) {                                     \
      positionIndex = (uint32_t)src_[i + build->positionOffset];             \
      if (positionIndex >= build->vertexCount)                               \
        goto done;                                                           \
                                                                             \
      found      = false;                                                    \
      chainProbe = 0;                                                        \
      slot       = lookup[positionIndex];                                    \
      while (slot) {                                                         \
        slot--;                                                              \
        if (EQFN(&src_[i], &src_[slot * build->indexStride],                 \
                 build->indexStride)) {                                      \
          ordinals[j] = ordinals[slot];                                      \
          found       = true;                                                \
          break;                                                             \
        }                                                                    \
                                                                             \
        if (++chainProbe >= AK_INDEX_CHAIN_PROBE_LIMIT) {                    \
          chainFallback = true;                                              \
          break;                                                             \
        }                                                                    \
                                                                             \
        slot = lookup[build->vertexCount + slot];                            \
      }                                                                      \
      if (chainFallback)                                                     \
        break;                                                               \
      if (found)                                                             \
        continue;                                                            \
                                                                             \
      lookup[build->vertexCount + j] = lookup[positionIndex];                \
      lookup[positionIndex]             = (uint32_t)j + 1;                   \
      AK_DUPLICATOR_BUILD_ASSIGN_NEW();                                      \
    }                                                                        \
                                                                             \
    if (chainFallback) {                                                     \
      memset(positionCopies, 0, build->vertexCount * 3 * sizeof(uint32_t));  \
      memset(lookup, 0, build->lookupCount * sizeof(uint32_t));              \
      positionNumber  = 0;                                                   \
      duplicateCount  = 0;                                                   \
      duplicateMax    = (AkUInt)build->vertexCount;                          \
                                                                             \
      for (j = i = 0; j < build->indexCount;                                \
           j++, i += build->indexStride) {                                   \
        positionIndex = (uint32_t)src_[i + build->positionOffset];           \
        if (positionIndex >= build->vertexCount)                             \
          goto done;                                                         \
        hashPosition = HASHFN(&src_[i], build->indexStride) & hashMask;      \
        for (;;) {                                                           \
          slot = lookup[hashPosition];                                       \
          if (!slot) {                                                       \
            lookup[hashPosition] = (uint32_t)j + 1;                          \
            AK_DUPLICATOR_BUILD_ASSIGN_NEW();                                \
            break;                                                           \
          }                                                                  \
                                                                             \
          slot--;                                                            \
          if (EQFN(&src_[i], &src_[slot * build->indexStride],               \
                   build->indexStride)) {                                    \
            ordinals[j] = ordinals[slot];                                    \
            break;                                                           \
          }                                                                  \
                                                                             \
          hashPosition = (hashPosition + 1) & hashMask;                      \
        }                                                                    \
      }                                                                      \
    }                                                                        \
  } while (0)

  switch (indices->componentType) {
    case AKT_UBYTE:
      AK_DUPLICATOR_BUILD_FOR_TYPE(uint8_t,
                                   ak_indexTupleHash8,
                                   ak_indexTupleEq8);
      break;
    case AKT_USHORT:
      AK_DUPLICATOR_BUILD_FOR_TYPE(uint16_t,
                                   ak_indexTupleHash16,
                                   ak_indexTupleEq16);
      break;
    case AKT_UINT:
      AK_DUPLICATOR_BUILD_FOR_TYPE(AkUInt,
                                   ak_indexTupleHash,
                                   ak_indexTupleEq);
      break;
    default:
      goto done;
  }

  build->duplicateCount = duplicateCount;
  build->bufferCount    = positionNumber;
  build->duplicateMax   = duplicateMax;
  build->valid          = true;

done:
#undef AK_DUPLICATOR_BUILD_FOR_TYPE
#undef AK_DUPLICATOR_BUILD_ASSIGN_NEW
  return;
}

AK_HIDE
AkDuplicator*
ak_meshDuplicatorBuildFinish(AkMesh            * __restrict mesh,
                             AkMeshPrimitive   * __restrict prim,
                             bool                           retain,
                             AkDuplicatorBuild * __restrict build) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkObject          *meshobj;
  AkDuplicator      *dupl;
  AkDuplicatorRange *range;
  AkIndexArray      *newIndices, *positionCopies, *copySums;
  AkTypeId           copyType, sumType;
  size_t             i, sumCount;

  if (!mesh
      || !prim
      || !build
      || !build->computed
      || !build->valid
      || build->primitive != prim
      || build->indices != prim->indices
      || !prim->pos
      || build->positionAccessor != prim->pos->accessor
      || build->indexStride != (prim->indexStride ? prim->indexStride : 1)
      || build->positionOffset != prim->pos->indexOffset)
    return NULL;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  doc     = ak_heap_data(heap);

  if (retain) {
    if ((dupl = ak__docDuplicatorFind(doc, prim))) {
      ak_free(dupl);
      (void)ak__docDuplicatorSet(doc, prim, NULL);
    }
  }

  dupl = ak_heap_calloc(heap, NULL, sizeof(*dupl));
  if (!dupl)
    return NULL;

  newIndices = ak_meshIndicesArrayFor(mesh, prim, true);
  if (!newIndices)
    goto fail;

#define AK_DUPLICATOR_BUILD_COPY_TO(TYPE, DST, SRC, COUNT)                  \
  do {                                                                       \
    TYPE *dst_;                                                              \
                                                                             \
    dst_ = (TYPE *)(void *)(DST);                                            \
    for (i = 0; i < (COUNT); i++)                                            \
      dst_[i] = (TYPE)(SRC)[i];                                              \
  } while (0)

  switch (newIndices->componentType) {
    case AKT_UBYTE:
      AK_DUPLICATOR_BUILD_COPY_TO(uint8_t,
                                  newIndices->items,
                                  build->ordinals,
                                  build->indexCount);
      break;
    case AKT_USHORT:
      AK_DUPLICATOR_BUILD_COPY_TO(uint16_t,
                                  newIndices->items,
                                  build->ordinals,
                                  build->indexCount);
      break;
    case AKT_UINT:
      memcpy(newIndices->items,
             build->ordinals,
             build->indexCount * sizeof(uint32_t));
      break;
    default:
      goto fail;
  }

  copyType = ak_indexComponentTypeForMax(build->duplicateMax);
  positionCopies = ak_indexArrayAlloc(heap,
                                      dupl,
                                      build->vertexCount * 3,
                                      copyType);
  if (!positionCopies)
    goto fail;
  positionCopies->max = build->duplicateMax;

  switch (copyType) {
    case AKT_UBYTE:
      AK_DUPLICATOR_BUILD_COPY_TO(uint8_t,
                                  positionCopies->items,
                                  build->positionCopies,
                                  build->vertexCount * 3);
      break;
    case AKT_USHORT:
      AK_DUPLICATOR_BUILD_COPY_TO(uint16_t,
                                  positionCopies->items,
                                  build->positionCopies,
                                  build->vertexCount * 3);
      break;
    case AKT_UINT:
      memcpy(positionCopies->items,
             build->positionCopies,
             build->vertexCount * 3 * sizeof(uint32_t));
      break;
    default:
      goto fail;
  }

#undef AK_DUPLICATOR_BUILD_COPY_TO

  sumType  = ak_indexComponentTypeForMax((AkUInt)build->duplicateCount);
  sumCount = build->bufferCount + 1;
  copySums = ak_indexSideArrayAllocZero(heap,
                                        positionCopies,
                                        sumCount,
                                        sumType);
  if (!copySums)
    goto fail;
  copySums->max = (AkUInt)build->duplicateCount;

#define AK_DUPLICATOR_BUILD_SUMS(TYPE)                                      \
  do {                                                                       \
    TYPE   *sums_;                                                           \
    AkUInt  position_, duplicate_, sum_;                                     \
                                                                             \
    sums_ = (TYPE *)(void *)copySums->items;                                 \
    for (i = 0; i < build->vertexCount; i++) {                               \
      if (build->positionCopies[3 * i + 2] == 0)                             \
        continue;                                                            \
      position_  = build->positionCopies[3 * i];                             \
      duplicate_ = build->positionCopies[3 * i + 1];                         \
      sums_[position_ + 1] = (TYPE)duplicate_;                               \
    }                                                                        \
    for (i = 1; i < copySums->count; i++) {                                  \
      sum_     = (AkUInt)sums_[i] + (AkUInt)sums_[i - 1];                    \
      sums_[i] = (TYPE)sum_;                                                 \
    }                                                                        \
  } while (0)

  switch (sumType) {
    case AKT_UBYTE:  AK_DUPLICATOR_BUILD_SUMS(uint8_t);  break;
    case AKT_USHORT: AK_DUPLICATOR_BUILD_SUMS(uint16_t); break;
    case AKT_UINT:   AK_DUPLICATOR_BUILD_SUMS(AkUInt);   break;
    default: goto fail;
  }

#undef AK_DUPLICATOR_BUILD_SUMS

  range = ak_heap_alloc(heap, dupl, sizeof(*range));
  if (!range)
    goto fail;

  range->next       = NULL;
  range->dupc       = positionCopies;
  range->dupcsum    = copySums;
  range->startIndex = 0;
  range->endIndex   = build->positionAccessor->count;

  dupl->range    = range;
  dupl->dupCount = build->duplicateCount;
  dupl->bufCount = build->bufferCount;

  if (retain && !ak__docDuplicatorSet(doc, prim, dupl))
    goto fail;

  return dupl;

fail:
  ak_free(dupl);
  return NULL;
}

AK_HIDE
void
ak_meshDuplicatorBuildRelease(AkDuplicatorBuild *build) {
  if (!build)
    return;

  if (build->ownsStorage)
    free(build->storage);
  memset(build, 0, sizeof(*build));
}

AK_HIDE
AkDuplicator*
ak_meshDuplicatorForIndicesRetained(AkMesh          * __restrict mesh,
                                    AkMeshPrimitive * __restrict prim,
                                    bool                         retain) {
  AkDuplicatorBuild build;
  AkDuplicator     *duplicator;

  if (!ak_meshDuplicatorBuildPrepare(prim, &build))
    return ak_meshDuplicatorForIndicesRetainedLegacy(mesh, prim, retain);

  ak_meshDuplicatorBuildCompute(&build);
  duplicator = ak_meshDuplicatorBuildFinish(mesh, prim, retain, &build);
  ak_meshDuplicatorBuildRelease(&build);
  if (duplicator)
    return duplicator;

  return ak_meshDuplicatorForIndicesRetainedLegacy(mesh, prim, retain);
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
