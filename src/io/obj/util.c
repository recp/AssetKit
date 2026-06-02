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

#include "util.h"
#include "../../strpool.h"

#define wobj_real_index(count, val)                                           \
  ((AkUInt)((val) > 0                                                       \
            ? ((AkUInt)(val) <= (AkUInt)(count) ? (val) - 1 : 0)             \
            : ((AkInt)(count) + (val) >= 0 ? (AkInt)(count) + (val) : 0)))

#define wobj_optional_index(count, val, fallback, useFallback)                \
  ((AkUInt)((val) == 0 && (useFallback)                                      \
            ? (fallback)                                                      \
            : wobj_real_index((count), (val))))

#define WOBJ_DIRECT_FLATTEN_MIN 4096

typedef struct WObjIndexSlot {
  uint32_t key;
  uint32_t compact;
} WObjIndexSlot;

#define WOBJ_JOIN_INDICES(TYPE)                                               \
  do {                                                                        \
    TYPE *dst_;                                                               \
                                                                              \
    dst_ = (TYPE *)(void *)prim->indices->items;                              \
                                                                              \
    if (wp->hasNormal && wp->hasTexture) {                                    \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          dst_[0] = (TYPE)wobj_real_index(count_pos, val);                    \
                                                                              \
          val = it2[1];                                                       \
          dst_[1] = (TYPE)wobj_optional_index(count_tex,                       \
                                              val,                            \
                                              wp->defaultTexIndex,            \
                                              wp->useDefaultTexture);         \
                                                                              \
          val = it2[2];                                                       \
          dst_[2] = (TYPE)wobj_optional_index(count_nor,                       \
                                              val,                            \
                                              wp->defaultNorIndex,            \
                                              wp->useDefaultNormal);          \
                                                                              \
          dst_ += 3;                                                          \
          it2  += 3;                                                          \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    } else if (wp->hasNormal) {                                               \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          dst_[0] = (TYPE)wobj_real_index(count_pos, val);                    \
                                                                              \
          val = it2[2];                                                       \
          dst_[1] = (TYPE)wobj_optional_index(count_nor,                       \
                                              val,                            \
                                              wp->defaultNorIndex,            \
                                              wp->useDefaultNormal);          \
                                                                              \
          dst_ += 2;                                                          \
          it2  += 3;                                                          \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    } else if (wp->hasTexture) {                                              \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          dst_[0] = (TYPE)wobj_real_index(count_pos, val);                    \
                                                                              \
          val = it2[1];                                                       \
          dst_[1] = (TYPE)wobj_optional_index(count_tex,                       \
                                              val,                            \
                                              wp->defaultTexIndex,            \
                                              wp->useDefaultTexture);         \
                                                                              \
          dst_ += 2;                                                          \
          it2  += 3;                                                          \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    } else {                                                                  \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          dst_[0] = (TYPE)wobj_real_index(count_pos, val);                    \
                                                                              \
          dst_ += 1;                                                          \
          it2  += 3;                                                          \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    }                                                                         \
  } while (0)

/* Keep exact actual-max narrowing for small/sparse OBJ primitives without
   adding a second full pass to large OBJ imports. */
#define WOBJ_SCAN_SHRINK_THRESHOLD 4096

#define WOBJ_SCAN_INDEX(VALUE_COUNT, FALLBACK, USE_FALLBACK)                  \
  do {                                                                        \
    AkUInt real_;                                                             \
                                                                              \
    real_ = wobj_optional_index((VALUE_COUNT),                                \
                                val,                                          \
                                (FALLBACK),                                   \
                                (USE_FALLBACK));                              \
    if (real_ > maxIndex)                                                     \
      maxIndex = real_;                                                       \
  } while (0)

#define WOBJ_SCAN_INDICES                                                     \
  do {                                                                        \
    if (wp->hasNormal && wp->hasTexture) {                                    \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          WOBJ_SCAN_INDEX(count_pos, 0, false);                               \
          val = it2[1];                                                       \
          WOBJ_SCAN_INDEX(count_tex,                                          \
                          wp->defaultTexIndex,                                \
                          wp->useDefaultTexture);                             \
          val = it2[2];                                                       \
          WOBJ_SCAN_INDEX(count_nor,                                          \
                          wp->defaultNorIndex,                                \
                          wp->useDefaultNormal);                              \
          it2 += 3;                                                           \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    } else if (wp->hasNormal) {                                               \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          WOBJ_SCAN_INDEX(count_pos, 0, false);                               \
          val = it2[2];                                                       \
          WOBJ_SCAN_INDEX(count_nor,                                          \
                          wp->defaultNorIndex,                                \
                          wp->useDefaultNormal);                              \
          it2 += 3;                                                           \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    } else if (wp->hasTexture) {                                              \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          WOBJ_SCAN_INDEX(count_pos, 0, false);                               \
          val = it2[1];                                                       \
          WOBJ_SCAN_INDEX(count_tex,                                          \
                          wp->defaultTexIndex,                                \
                          wp->useDefaultTexture);                             \
          it2 += 3;                                                           \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    } else {                                                                  \
      while (chunk) {                                                         \
        csz = chunk->usedsize;                                                \
        it2 = (void *)chunk->data;                                            \
                                                                              \
        for (i = 0; i < csz; i += isz) {                                      \
          val = it2[0];                                                       \
          WOBJ_SCAN_INDEX(count_pos, 0, false);                               \
          it2 += 3;                                                           \
        }                                                                     \
        chunk = chunk->next;                                                  \
      }                                                                       \
    }                                                                         \
  } while (0)

AK_HIDE
AkAccessor*
wobj_acc(WOState         * __restrict wst,
         AkDataContext   * __restrict dctx,
         AkComponentSize              compSize,
         AkTypeId                     type) {
  AkHeap     *heap;
  AkBuffer   *buff;
  AkAccessor *acc;
  AkTypeDesc *typeDesc;
  int         nComponents;

  heap        = wst->heap;
  typeDesc    = ak_typeDesc(type);
  nComponents = (int)compSize;

  buff         = ak_heap_calloc(heap, wst->doc, sizeof(*buff));
  buff->data   = ak_heap_alloc(heap, buff, dctx->usedsize);
  buff->length = dctx->usedsize;
  ak_data_join(dctx, buff->data, 0, 0);
  
  AK_LIB_PREPEND(wst->doc->lib.buffers, buff, next);
  
  acc                    = ak_heap_calloc(heap, wst->doc, sizeof(*acc));
  acc->buffer            = buff;
  acc->byteLength        = buff->length;
  acc->byteStride        = typeDesc->size * nComponents;
  acc->componentSize     = compSize;
  acc->componentType          = type;
  acc->originalComponentType  = type;
  acc->bytesPerComponent      = typeDesc->size;
  acc->componentCount         = nComponents;
  acc->fillByteSize           = typeDesc->size * nComponents;
  acc->count                  = (uint32_t)dctx->itemcount;

  return acc;
}

AK_HIDE
AkInput*
wobj_input(WOState         * __restrict wst,
           AkMeshPrimitive * __restrict prim,
           AkAccessor      * __restrict acc,
           AkInputSemantic              sem,
           const char      * __restrict semRaw,
           uint32_t                     offset) {
  AkInput *inp;

  inp              = ak_heap_calloc(wst->heap, prim, sizeof(*inp));
  inp->accessor    = acc;
  inp->semantic    = sem;
  inp->semanticRaw = ak_heap_strdup(wst->heap, inp, semRaw);
  inp->indexOffset = offset;

  inp->next   = prim->input;
  prim->input = inp;
  prim->inputCount++;
  
  ak_retain(acc);

  return inp;
}

static
AkInput*
wobj_flatInput(WOState         * __restrict wst,
               AkMeshPrimitive * __restrict prim,
               AkInputSemantic              sem,
               const char      * __restrict semRaw,
               AkComponentSize              compSize,
               AkTypeId                     type,
               uint32_t                     count) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkBuffer   *buff;
  AkAccessor *acc;
  AkTypeDesc *typeDesc;
  int         nComponents;

  heap        = wst->heap;
  doc         = wst->doc;
  typeDesc    = ak_typeDesc(type);
  nComponents = (int)compSize;

  buff         = ak_heap_calloc(heap, doc, sizeof(*buff));
  buff->length = typeDesc->size * nComponents * count;
  buff->data   = ak_heap_alloc(heap, buff, buff->length);

  AK_LIB_PREPEND(doc->lib.buffers, buff, next);

  acc                    = ak_heap_calloc(heap, doc, sizeof(*acc));
  acc->buffer            = buff;
  acc->byteLength        = buff->length;
  acc->byteStride        = typeDesc->size * nComponents;
  acc->componentSize     = compSize;
  acc->componentType          = type;
  acc->originalComponentType  = type;
  acc->bytesPerComponent      = typeDesc->size;
  acc->componentCount         = nComponents;
  acc->fillByteSize           = typeDesc->size * nComponents;
  acc->count                  = count;
  AK_LIB_PREPEND(doc->lib.accessors, acc, next);

  return wobj_input(wst, prim, acc, sem, semRaw, 0);
}

AK_HIDE
bool
wobj_flattenPrimDirect(WOState         * __restrict wst,
                       WOPrim          * __restrict wp,
                       AkMeshPrimitive * __restrict prim) {
  AkAccessor    *posAcc, *texAcc, *norAcc, *colAcc;
  AkInput       *posInp, *texInp, *norInp, *colInp;
  AkDataChunk   *chunk;
  const AkInt   *face;
  char          *posSrc, *texSrc, *norSrc, *colSrc;
  char          *posDst, *texDst, *norDst, *colDst;
  size_t         nfaces, i, count, posBytes, texBytes, norBytes, colBytes;
  uint32_t       posCount, texCount, norCount, colCount;
  AkUInt         posIdx, texIdx, norIdx;

  if (!wp->dc_face
      || !wp->dc_face->data
      || wp->dc_face->itemcount < WOBJ_DIRECT_FLATTEN_MIN
      || (!wp->hasTexture && !wp->hasNormal)
      || !(posAcc = wst->ac_pos)
      || !posAcc->buffer)
    return false;

  texAcc = wst->ac_tex;
  norAcc = wst->ac_nor;
  colAcc = wst->ac_col;
  if (wp->hasTexture && (!texAcc || !texAcc->buffer))
    return false;
  if (wp->hasNormal && (!norAcc || !norAcc->buffer))
    return false;

  posCount = posAcc->count;
  texCount = texAcc ? texAcc->count : 0;
  norCount = norAcc ? norAcc->count : 0;
  colCount = colAcc ? colAcc->count : 0;
  if (posCount == 0
      || (wp->hasTexture && texCount == 0)
      || (wp->hasNormal && norCount == 0)
      || (colAcc && colCount < posCount))
    return false;

  posBytes = posAcc->fillByteSize;
  texBytes = texAcc ? texAcc->fillByteSize : 0;
  norBytes = norAcc ? norAcc->fillByteSize : 0;
  colBytes = colAcc ? colAcc->fillByteSize : 0;

  count  = wp->dc_face->itemcount;
  posInp = wobj_flatInput(wst,
                          prim,
                          AK_INPUT_POSITION,
                          _s_POSITION,
                          posAcc->componentSize,
                          AKT_FLOAT,
                          (uint32_t)count);
  prim->pos = posInp;

  colInp = NULL;
  if (colAcc) {
    colInp = wobj_flatInput(wst,
                            prim,
                            AK_INPUT_COLOR,
                            _s_COLOR,
                            colAcc->componentSize,
                            AKT_FLOAT,
                            (uint32_t)count);
  }

  texInp = NULL;
  if (wp->hasTexture) {
    texInp = wobj_flatInput(wst,
                            prim,
                            AK_INPUT_TEXCOORD,
                            _s_TEXCOORD,
                            texAcc->componentSize,
                            AKT_FLOAT,
                            (uint32_t)count);
  }

  norInp = NULL;
  if (wp->hasNormal) {
    norInp = wobj_flatInput(wst,
                            prim,
                            AK_INPUT_NORMAL,
                            _s_NORMAL,
                            AK_COMPONENT_SIZE_VEC3,
                            AKT_FLOAT,
                            (uint32_t)count);
  }

  posSrc = (char *)posAcc->buffer->data + posAcc->byteOffset;
  texSrc = texAcc && texAcc->buffer
           ? (char *)texAcc->buffer->data + texAcc->byteOffset
           : NULL;
  norSrc = norAcc && norAcc->buffer
           ? (char *)norAcc->buffer->data + norAcc->byteOffset
           : NULL;
  colSrc = colAcc && colAcc->buffer
           ? (char *)colAcc->buffer->data + colAcc->byteOffset
           : NULL;
  posDst = (char *)posInp->accessor->buffer->data;
  texDst = texInp ? (char *)texInp->accessor->buffer->data : NULL;
  norDst = norInp ? (char *)norInp->accessor->buffer->data : NULL;
  colDst = colInp ? (char *)colInp->accessor->buffer->data : NULL;

#define WOBJ_FLATTEN_COPY(COPY_TEX, COPY_NOR)                                \
  do {                                                                        \
    chunk = wp->dc_face->data;                                                \
    while (chunk) {                                                           \
      face   = (const AkInt *)(const void *)chunk->data;                      \
      nfaces = chunk->usedsize / sizeof(ivec3);                               \
      for (i = 0; i < nfaces; i++, face += 3) {                               \
        posIdx = wobj_real_index(posCount, face[0]);                          \
        memcpy(posDst, posSrc + posAcc->byteStride * posIdx, posBytes);       \
        posDst += posBytes;                                                   \
                                                                              \
        if (colDst) {                                                         \
          memcpy(colDst, colSrc + colAcc->byteStride * posIdx, colBytes);     \
          colDst += colBytes;                                                 \
        }                                                                     \
                                                                              \
        if (COPY_TEX) {                                                       \
          texIdx = wobj_optional_index(texCount,                               \
                                       face[1],                               \
                                       wp->defaultTexIndex,                   \
                                       wp->useDefaultTexture);                \
          memcpy(texDst, texSrc + texAcc->byteStride * texIdx, texBytes);     \
          texDst += texBytes;                                                 \
        }                                                                     \
                                                                              \
        if (COPY_NOR) {                                                       \
          norIdx = wobj_optional_index(norCount,                               \
                                       face[2],                               \
                                       wp->defaultNorIndex,                   \
                                       wp->useDefaultNormal);                 \
          memcpy(norDst, norSrc + norAcc->byteStride * norIdx, norBytes);     \
          norDst += norBytes;                                                 \
        }                                                                     \
      }                                                                       \
      chunk = chunk->next;                                                    \
    }                                                                         \
  } while (0)

  if (texDst && norDst) {
    WOBJ_FLATTEN_COPY(true, true);
  } else if (texDst) {
    WOBJ_FLATTEN_COPY(true, false);
  } else {
    WOBJ_FLATTEN_COPY(false, true);
  }

#undef WOBJ_FLATTEN_COPY

  prim->indices       = NULL;
  prim->indexAccessor = NULL;
  prim->indexStride   = 1;

  return true;
}

static
uint32_t
wobj_index_hash_u32(uint32_t value) {
  return value * 2654435761u;
}

static
size_t
wobj_index_table_capacity(size_t count) {
  size_t cap, wanted;

  wanted = count + (count >> 1) + 1;
  cap    = 16;

  while (cap < wanted) {
    if (cap > SIZE_MAX / 2)
      return 0;
    cap <<= 1;
  }

  return cap;
}

static
void
wobj_copy_compact_indices(AkIndexArray       * __restrict indices,
                          const uint32_t     * __restrict src,
                          size_t                          count) {
  size_t i;

  switch (indices->componentType) {
    case AKT_UBYTE: {
      uint8_t *dst;

      dst = (uint8_t *)indices->items;
      for (i = 0; i < count; i++)
        dst[i] = (uint8_t)src[i];
      break;
    }
    case AKT_USHORT: {
      uint16_t *dst;

      dst = (uint16_t *)indices->items;
      for (i = 0; i < count; i++)
        dst[i] = (uint16_t)src[i];
      break;
    }
    case AKT_UINT: {
      uint32_t *dst;

      dst = (uint32_t *)indices->items;
      memcpy(dst, src, sizeof(*dst) * count);
      break;
    }
    default:
      break;
  }
}

static
void
wobj_copy_compact_accessor(AkAccessor    * __restrict dstAcc,
                           AkAccessor    * __restrict srcAcc,
                           const uint32_t * __restrict compactSources,
                           uint32_t                    uniqueCount) {
  char    *dst, *src;
  size_t   bytes, srcStride, i;

  dst       = (char *)dstAcc->buffer->data;
  src       = (char *)srcAcc->buffer->data + srcAcc->byteOffset;
  bytes     = srcAcc->fillByteSize;
  srcStride = srcAcc->byteStride;

  for (i = 0; i < uniqueCount; i++) {
    memcpy(dst, src + srcStride * compactSources[i], bytes);
    dst += bytes;
  }
}

AK_HIDE
bool
wobj_compactIndexedPointPrim(WOState         * __restrict wst,
                             WOPrim          * __restrict wp,
                             AkMeshPrimitive * __restrict prim) {
  AkAccessor    *posAcc, *colAcc;
  AkInput       *posInp, *colInp;
  AkIndexArray  *indices;
  AkDataChunk   *chunk;
  WObjIndexSlot *table, *slot;
  uint32_t      *tmpIndices, *compactSources;
  const AkInt   *face;
  size_t         count, uniqueCapacity, tableCap, tableMask, nitems, i, outIndex;
  uint32_t       posCount, uniqueCount;
  AkTypeId       componentType;

  if (wp->kind != AK_PRIMITIVE_LINES && wp->kind != AK_PRIMITIVE_POINTS)
    return false;
  if (!wp->dc_face || !wp->dc_face->data || wp->dc_face->itemcount == 0)
    return false;
  if (!(posAcc = wst->ac_pos) || !posAcc->buffer || !posAcc->buffer->data)
    return false;

  count    = wp->dc_face->itemcount;
  posCount = posAcc->count;
  if (posCount == 0 || count > UINT32_MAX)
    return false;

  colAcc = wst->ac_col;
  if (colAcc && (!colAcc->buffer || !colAcc->buffer->data || colAcc->count < posCount))
    return false;

  uniqueCapacity = count < posCount ? count : posCount;
  tableCap = wobj_index_table_capacity(uniqueCapacity);
  if (tableCap == 0)
    return false;
  tableMask = tableCap - 1;

  table = ak_heap_calloc(wst->heap, wst->tmp, sizeof(*table) * tableCap);
  tmpIndices = ak_heap_alloc(wst->heap, wst->tmp, sizeof(*tmpIndices) * count);
  compactSources = ak_heap_alloc(wst->heap,
                                 wst->tmp,
                                 sizeof(*compactSources) * uniqueCapacity);
  if (!table || !tmpIndices || !compactSources)
    return false;

  uniqueCount = 0;
  outIndex    = 0;
  chunk       = wp->dc_face->data;
  while (chunk) {
    face   = (const AkInt *)(const void *)chunk->data;
    nitems = chunk->usedsize / sizeof(ivec3);
    for (i = 0; i < nitems; i++, face += 3) {
      uint32_t source, key, compact;
      size_t   hash;

      source = (uint32_t)wobj_real_index(posCount, face[0]);
      key    = source + 1u;
      hash   = (size_t)wobj_index_hash_u32(key) & tableMask;

      for (;;) {
        slot = &table[hash];
        if (slot->key == key) {
          compact = slot->compact;
          break;
        }
        if (slot->key == 0) {
          compact                = uniqueCount;
          slot->key              = key;
          slot->compact          = compact;
          compactSources[compact] = source;
          uniqueCount++;
          if (uniqueCount >= posCount)
            return false;
          break;
        }
        hash = (hash + 1) & tableMask;
      }

      tmpIndices[outIndex++] = compact;
    }
    chunk = chunk->next;
  }

  if (uniqueCount >= posCount)
    return false;

  componentType     = ak_indexComponentTypeForMax(uniqueCount > 0 ? uniqueCount - 1 : 0);
  indices           = ak_indexArrayAlloc(wst->heap, prim, count, componentType);
  if (!indices)
    return false;
  indices->max = uniqueCount > 0 ? uniqueCount - 1 : 0;
  wobj_copy_compact_indices(indices, tmpIndices, count);

  posInp = wobj_flatInput(wst,
                          prim,
                          AK_INPUT_POSITION,
                          _s_POSITION,
                          posAcc->componentSize,
                          posAcc->componentType,
                          uniqueCount);
  prim->pos = posInp;

  if (colAcc) {
    colInp = wobj_flatInput(wst,
                            prim,
                            AK_INPUT_COLOR,
                            _s_COLOR,
                            colAcc->componentSize,
                            colAcc->componentType,
                            uniqueCount);
    wobj_copy_compact_accessor(colInp->accessor, colAcc, compactSources, uniqueCount);
  }

  wobj_copy_compact_accessor(posInp->accessor, posAcc, compactSources, uniqueCount);

  prim->indices       = indices;
  prim->indexAccessor = NULL;
  prim->indexStride   = 1;

  return true;
}

AK_HIDE
void
wobj_joinIndices(WOState         * __restrict wst,
                 WOPrim          * __restrict wp,
                 AkMeshPrimitive * __restrict prim) {
  AkDataChunk *chunk;
  AkInt       *it2, val;
  AkUInt       maxIndex;
  AkTypeId     componentType;
  size_t       count;
  size_t       isz, csz, i;
  uint32_t     istride, count_pos, count_tex, count_nor;

  if (!wp->dc_face->data)
    return;
  
  count   = wp->dc_face->itemcount;
  istride = 1;

  count_pos = wst->ac_pos ? wst->ac_pos->count : 0;
  count_tex = wst->ac_tex ? wst->ac_tex->count : 0;
  count_nor = wst->ac_nor ? wst->ac_nor->count : 0;
  maxIndex  = count_pos > 0 ? count_pos - 1 : 0;
  if (wp->hasTexture && count_tex > 0 && count_tex - 1 > maxIndex)
    maxIndex = count_tex - 1;
  if (wp->hasNormal && count_nor > 0 && count_nor - 1 > maxIndex)
    maxIndex = count_nor - 1;

  if (wp->hasTexture || wp->hasNormal) {
    istride += (int)wp->hasNormal + (int)wp->hasTexture;
    count   *= istride;
  }

  isz = wp->dc_face->itemsize;
  if (count <= WOBJ_SCAN_SHRINK_THRESHOLD) {
    chunk    = wp->dc_face->data;
    maxIndex = 0;
    WOBJ_SCAN_INDICES;
  }

  componentType     = ak_indexComponentTypeForMax(maxIndex);
  prim->indices     = ak_indexArrayAlloc(wst->heap, prim, count, componentType);
  prim->indexStride = istride;
  if (!prim->indices)
    return;

  prim->indices->max = maxIndex;

  /* join index buffer chunks */
  chunk = wp->dc_face->data;

  /* to make it faster split cases */
  switch (componentType) {
    case AKT_UBYTE:  WOBJ_JOIN_INDICES(uint8_t);  break;
    case AKT_USHORT: WOBJ_JOIN_INDICES(uint16_t); break;
    case AKT_UINT:   WOBJ_JOIN_INDICES(uint32_t); break;
    default:         break;
  }
}

#undef WOBJ_SCAN_INDICES
#undef WOBJ_SCAN_INDEX
#undef WOBJ_SCAN_SHRINK_THRESHOLD
#undef WOBJ_DIRECT_FLATTEN_MIN
#undef wobj_optional_index
