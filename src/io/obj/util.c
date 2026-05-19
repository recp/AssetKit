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
  ((AkUInt)((val) > 0 ? (val) - 1 : (AkInt)(count) + (val)))

#define wobj_optional_index(count, val, fallback, useFallback)                \
  ((AkUInt)((val) == 0 && (useFallback)                                      \
            ? (fallback)                                                      \
            : wobj_real_index((count), (val))))

#define WOBJ_DIRECT_FLATTEN_MIN 4096

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
  
  flist_sp_insert(&wst->doc->lib.buffers, buff);
  
  acc                    = ak_heap_calloc(heap, wst->doc, sizeof(*acc));
  acc->buffer            = buff;
  acc->byteLength        = buff->length;
  acc->byteStride        = typeDesc->size * nComponents;
  acc->componentSize     = compSize;
  acc->componentType     = type;
  acc->bytesPerComponent = typeDesc->size;
  acc->componentCount    = nComponents;
  acc->fillByteSize      = typeDesc->size * nComponents;
  acc->count             = (uint32_t)dctx->itemcount;

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

  inp                 = ak_heap_calloc(wst->heap, prim, sizeof(*inp));
  inp->accessor       = acc;
  inp->semantic       = sem;
  inp->semanticRaw    = ak_heap_strdup(wst->heap, inp, semRaw);
  inp->offset         = offset;

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

  flist_sp_insert(&doc->lib.buffers, buff);

  acc                    = ak_heap_calloc(heap, doc, sizeof(*acc));
  acc->buffer            = buff;
  acc->byteLength        = buff->length;
  acc->byteStride        = typeDesc->size * nComponents;
  acc->componentSize     = compSize;
  acc->componentType     = type;
  acc->bytesPerComponent = typeDesc->size;
  acc->componentCount    = nComponents;
  acc->fillByteSize      = typeDesc->size * nComponents;
  acc->count             = count;

  return wobj_input(wst, prim, acc, sem, semRaw, 0);
}

AK_HIDE
bool
wobj_flattenPrimDirect(WOState         * __restrict wst,
                       WOPrim          * __restrict wp,
                       AkMeshPrimitive * __restrict prim) {
  AkAccessor    *posAcc, *texAcc, *norAcc;
  AkInput       *posInp, *texInp, *norInp;
  AkDataChunk   *chunk;
  const AkInt   *face;
  char          *posSrc, *texSrc, *norSrc;
  char          *posDst, *texDst, *norDst;
  size_t         nfaces, i, count;
  uint32_t       posCount, texCount, norCount;
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
  if (wp->hasTexture && (!texAcc || !texAcc->buffer))
    return false;
  if (wp->hasNormal && (!norAcc || !norAcc->buffer))
    return false;

  posCount = posAcc->count;
  texCount = texAcc ? texAcc->count : 0;
  norCount = norAcc ? norAcc->count : 0;
  if (posCount == 0
      || (wp->hasTexture && texCount == 0)
      || (wp->hasNormal && norCount == 0))
    return false;

  count  = wp->dc_face->itemcount;
  posInp = wobj_flatInput(wst,
                          prim,
                          AK_INPUT_POSITION,
                          _s_POSITION,
                          AK_COMPONENT_SIZE_VEC3,
                          AKT_FLOAT,
                          (uint32_t)count);
  prim->pos = posInp;

  texInp = NULL;
  if (wp->hasTexture) {
    texInp = wobj_flatInput(wst,
                            prim,
                            AK_INPUT_TEXCOORD,
                            _s_TEXCOORD,
                            AK_COMPONENT_SIZE_VEC2,
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
  posDst = (char *)posInp->accessor->buffer->data;
  texDst = texInp ? (char *)texInp->accessor->buffer->data : NULL;
  norDst = norInp ? (char *)norInp->accessor->buffer->data : NULL;

#define WOBJ_FLATTEN_COPY(COPY_TEX, COPY_NOR)                                \
  do {                                                                        \
    chunk = wp->dc_face->data;                                                \
    while (chunk) {                                                           \
      face   = (const AkInt *)(const void *)chunk->data;                      \
      nfaces = chunk->usedsize / sizeof(ivec3);                               \
      for (i = 0; i < nfaces; i++, face += 3) {                               \
        posIdx = wobj_real_index(posCount, face[0]);                          \
        memcpy(posDst, posSrc + posAcc->byteStride * posIdx, sizeof(vec3));   \
        posDst += sizeof(vec3);                                               \
                                                                              \
        if (COPY_TEX) {                                                       \
          texIdx = wobj_optional_index(texCount,                               \
                                       face[1],                               \
                                       wp->defaultTexIndex,                   \
                                       wp->useDefaultTexture);                \
          memcpy(texDst, texSrc + texAcc->byteStride * texIdx, sizeof(vec2)); \
          texDst += sizeof(vec2);                                             \
        }                                                                     \
                                                                              \
        if (COPY_NOR) {                                                       \
          norIdx = wobj_optional_index(norCount,                               \
                                       face[2],                               \
                                       wp->defaultNorIndex,                   \
                                       wp->useDefaultNormal);                 \
          memcpy(norDst, norSrc + norAcc->byteStride * norIdx, sizeof(vec3)); \
          norDst += sizeof(vec3);                                             \
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
