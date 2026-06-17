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

#ifndef ply_util_h
#define ply_util_h

#include "common.h"
#include "../../endian.h"

typedef struct PLYTriSeenSlot {
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t used;
} PLYTriSeenSlot;

typedef struct PLYTriSeen {
  uint64_t       *keys;
  PLYTriSeenSlot *slots;
  size_t          cap;
} PLYTriSeen;

#define ply_val(p, typeDesc, leEndian, T, DEST, DEFAULT)                      \
  do {                                                                        \
    uint64_t buf;                                                             \
                                                                              \
    buf = 0;                                                                  \
    switch (typeDesc->typeId) {                                               \
      case AKT_FLOAT:                                                         \
      case AKT_INT:                                                           \
      case AKT_UINT:   memcpy_endian32(leEndian, buf, p); break;              \
      case AKT_DOUBLE:                                                        \
      case AKT_INT64:                                                         \
      case AKT_UINT64: memcpy_endian64(leEndian, buf, p); break;              \
      case AKT_SHORT:                                                         \
      case AKT_USHORT: memcpy_endian16(leEndian, buf, p); break;              \
      case AKT_BYTE:                                                          \
      case AKT_UBYTE:  memcpy(&buf, p++, 1);              break;              \
      default:         DEST = DEFAULT;                    break;              \
    }                                                                         \
                                                                              \
    switch (typeDesc->typeId) {                                               \
      case AKT_FLOAT: {                                                       \
        float value;                                                          \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_INT: {                                                         \
        int32_t value;                                                        \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_UINT: {                                                        \
        uint32_t value;                                                       \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_DOUBLE: {                                                      \
        double value;                                                         \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_INT64: {                                                       \
        int64_t value;                                                        \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_UINT64: {                                                      \
        uint64_t value;                                                       \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_SHORT: {                                                       \
        int16_t value;                                                        \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_USHORT: {                                                      \
        uint16_t value;                                                       \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_BYTE: {                                                        \
        int8_t value;                                                         \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      case AKT_UBYTE: {                                                       \
        uint8_t value;                                                        \
        memcpy(&value, &buf, sizeof(value));                                  \
        DEST = (T)value;                                                      \
        break;                                                               \
      }                                                                       \
      default: DEST = DEFAULT; break;                                         \
    }                                                                         \
  } while (0)

static inline
AkDataContext*
ply_index_data_new_for_estimated(PLYState * __restrict pst,
                                 AkTypeId * __restrict componentType,
                                 AkUInt   * __restrict indexMax,
                                 size_t                  estimatedItems) {
  AkUInt maxIndex;
  size_t itemSize, nodeBytes, nodeItems;

  maxIndex = pst->vertcount > 0 ? pst->vertcount - 1 : 0;
  *componentType = ak_indexComponentTypeForMax(maxIndex);
  *indexMax = 0;
  itemSize  = ak_indexComponentSize(*componentType);
  nodeBytes = 1024;

  if (estimatedItems > 0) {
    if (estimatedItems > (64 * 1024) / itemSize)
      nodeBytes = 64 * 1024;
    else if (estimatedItems > (16 * 1024) / itemSize)
      nodeBytes = 16 * 1024;
  } else if (pst->vertcount > 65536) {
    nodeBytes = 64 * 1024;
  } else if (pst->vertcount > 4096) {
    nodeBytes = 16 * 1024;
  }

  nodeItems = nodeBytes / itemSize;
  if (nodeItems < 128)
    nodeItems = 128;

  return ak_data_new(pst->tmp, nodeItems, itemSize, NULL);
}

static inline
AkDataContext*
ply_index_data_new_for(PLYState * __restrict pst,
                       AkTypeId * __restrict componentType,
                       AkUInt   * __restrict indexMax) {
  return ply_index_data_new_for_estimated(pst, componentType, indexMax, 0);
}

static inline
size_t
ply_tristrip_seen_capacity(size_t count) {
  size_t cap, faces, wanted;

  if (count < 3 || count > SIZE_MAX / 2)
    return 0;

  faces  = count - 2;
  wanted = faces + faces / 2;
  cap    = 64;
  while (cap < wanted) {
    if (cap > SIZE_MAX / 2)
      return 0;
    cap <<= 1;
  }

  return cap;
}

static inline
uint64_t
ply_hash64(uint64_t value) {
  value ^= value >> 33;
  value *= 0xff51afd7ed558ccdull;
  value ^= value >> 33;
  return value;
}

static inline
uint32_t
ply_tri_hash(uint32_t a, uint32_t b, uint32_t c) {
  uint32_t value;

  value = a * 73856093u;
  value ^= b * 19349663u;
  value ^= c * 83492791u;
  value ^= value >> 16;

  return value;
}

static inline
void
ply_tri_sort3(uint32_t * __restrict a,
              uint32_t * __restrict b,
              uint32_t * __restrict c) {
  uint32_t t;

  if (*a > *b) {
    t = *a;
    *a = *b;
    *b = t;
  }
  if (*b > *c) {
    t = *b;
    *b = *c;
    *c = t;
  }
  if (*a > *b) {
    t = *a;
    *a = *b;
    *b = t;
  }
}

static inline
void
ply_tri_seen_init(PLYTriSeen * __restrict seen,
                  PLYState   * __restrict pst,
                  size_t                   count) {
  size_t cap;

  seen->keys  = NULL;
  seen->slots = NULL;
  seen->cap   = 0;

  cap = ply_tristrip_seen_capacity(count);
  if (cap == 0)
    return;

  seen->cap = cap;
  if (pst->vertcount <= 0x1fffffu) {
    seen->keys = ak_heap_calloc(pst->heap, pst->tmp, sizeof(*seen->keys) * cap);
  } else {
    seen->slots = ak_heap_calloc(pst->heap,
                                 pst->tmp,
                                 sizeof(*seen->slots) * cap);
  }
}

static inline
bool
ply_tri_seen_insert(PLYTriSeen * __restrict seen,
                    AkUInt                   a,
                    AkUInt                   b,
                    AkUInt                   c) {
  uint32_t sa, sb, sc;
  size_t   mask, pos;

  if (!seen || seen->cap == 0)
    return true;
  if (a > UINT32_MAX || b > UINT32_MAX || c > UINT32_MAX)
    return true;

  sa = (uint32_t)a;
  sb = (uint32_t)b;
  sc = (uint32_t)c;
  ply_tri_sort3(&sa, &sb, &sc);

  mask = seen->cap - 1;

  if (seen->keys) {
    uint64_t key;

    key = (((uint64_t)sa) << 42) | (((uint64_t)sb) << 21) | (uint64_t)sc;
    key++;
    pos = (size_t)ply_hash64(key) & mask;
    for (;;) {
      if (seen->keys[pos] == 0) {
        seen->keys[pos] = key;
        return true;
      }
      if (seen->keys[pos] == key)
        return false;
      pos = (pos + 1) & mask;
    }
  } else {
    PLYTriSeenSlot *slot;

    pos = (size_t)ply_tri_hash(sa, sb, sc) & mask;
    for (;;) {
      slot = &seen->slots[pos];
      if (!slot->used) {
        slot->a    = sa;
        slot->b    = sb;
        slot->c    = sc;
        slot->used = 1;
        return true;
      }
      if (slot->a == sa && slot->b == sb && slot->c == sc)
        return false;
      pos = (pos + 1) & mask;
    }
  }
}

static inline
void
ply_index_append_to(AkDataContext * __restrict dctx,
                    AkTypeId                    componentType,
                    AkUInt       * __restrict   indexMax,
                    AkUInt                      value) {
  if (value > *indexMax)
    *indexMax = value;

  switch (componentType) {
    case AKT_UBYTE: {
      uint8_t v;

      v = (uint8_t)value;
      ak_data_append(dctx, &v);
      break;
    }
    case AKT_USHORT: {
      uint16_t v;

      v = (uint16_t)value;
      ak_data_append(dctx, &v);
      break;
    }
    case AKT_UINT: {
      uint32_t v;

      v = (uint32_t)value;
      ak_data_append(dctx, &v);
      break;
    }
    default:
      break;
  }
}

static inline
AkDataContext*
ply_index_data_new(PLYState * __restrict pst) {
  return ply_index_data_new_for(pst, &pst->indexComponentType, &pst->indexMax);
}

static inline
AkDataContext*
ply_index_data_new_estimated(PLYState * __restrict pst,
                             size_t                  estimatedItems) {
  return ply_index_data_new_for_estimated(pst,
                                          &pst->indexComponentType,
                                          &pst->indexMax,
                                          estimatedItems);
}

static inline
void
ply_index_append(PLYState * __restrict pst, AkUInt value) {
  ply_index_append_to(pst->dc_ind,
                      pst->indexComponentType,
                      &pst->indexMax,
                      value);
}

static inline
void
ply_edge_append(PLYState * __restrict pst, AkUInt a, AkUInt b) {
  if (!pst->dc_edge_ind)
    pst->dc_edge_ind = ply_index_data_new_for(pst,
                                              &pst->edgeIndexComponentType,
                                              &pst->edgeIndexMax);

  ply_index_append_to(pst->dc_edge_ind,
                      pst->edgeIndexComponentType,
                      &pst->edgeIndexMax,
                      a);
  ply_index_append_to(pst->dc_edge_ind,
                      pst->edgeIndexComponentType,
                      &pst->edgeIndexMax,
                      b);
  pst->edgeIndexCount += 2;
}

#define PLY_INDEX_APPEND_TYPED(PST, TYPE, VALUE)                              \
  do {                                                                        \
    AkDataContext *dctx_;                                                     \
    AkDataChunk   *chunk_;                                                    \
    AkUInt value_;                                                            \
    TYPE   typed_;                                                            \
                                                                              \
    dctx_  = (PST)->dc_ind;                                                   \
    value_ = (VALUE);                                                         \
    if (value_ > (PST)->indexMax)                                             \
      (PST)->indexMax = value_;                                               \
    typed_ = (TYPE)value_;                                                    \
    if (!dctx_->last                                                           \
        || dctx_->last->usedsize + sizeof(TYPE) > dctx_->nodesize) {           \
      chunk_ = ak_heap_alloc(dctx_->heap,                                     \
                             dctx_,                                           \
                             sizeof(*chunk_) + dctx_->nodesize);              \
      chunk_->usedsize = 0;                                                   \
      chunk_->next     = NULL;                                                \
      if (dctx_->last)                                                        \
        dctx_->last->next = chunk_;                                           \
      dctx_->last  = chunk_;                                                  \
      dctx_->size += dctx_->nodesize;                                         \
      if (!dctx_->data)                                                       \
        dctx_->data = chunk_;                                                 \
    } else {                                                                  \
      chunk_ = dctx_->last;                                                   \
    }                                                                         \
    *(TYPE *)(void *)(chunk_->data + chunk_->usedsize) = typed_;              \
    chunk_->usedsize += sizeof(TYPE);                                         \
    dctx_->usedsize  += sizeof(TYPE);                                         \
    dctx_->itemcount++;                                                       \
  } while (0)

#define PLY_INDEX_APPEND_FACE_TYPED(PST, TYPE, FACE, FACE_COUNT, OUT_COUNT)   \
  do {                                                                        \
    AkUInt center_, j_;                                                       \
                                                                              \
    center_ = (FACE)[0];                                                      \
    for (j_ = 0; j_ < (FACE_COUNT) - 2; j_++) {                               \
      PLY_INDEX_APPEND_TYPED((PST), TYPE, center_);                           \
      PLY_INDEX_APPEND_TYPED((PST), TYPE, (FACE)[j_ + 1]);                    \
      PLY_INDEX_APPEND_TYPED((PST), TYPE, (FACE)[j_ + 2]);                    \
      (OUT_COUNT) += 3;                                                       \
    }                                                                         \
  } while (0)

#define PLY_INDEX_APPEND_FACE(PST, FACE, FACE_COUNT, OUT_COUNT)               \
  do {                                                                        \
    switch ((PST)->indexComponentType) {                                      \
      case AKT_UBYTE:                                                         \
        PLY_INDEX_APPEND_FACE_TYPED((PST), uint8_t,                           \
                                    (FACE), (FACE_COUNT), (OUT_COUNT));       \
        break;                                                                \
      case AKT_USHORT:                                                        \
        PLY_INDEX_APPEND_FACE_TYPED((PST), uint16_t,                          \
                                    (FACE), (FACE_COUNT), (OUT_COUNT));       \
        break;                                                                \
      case AKT_UINT:                                                          \
        PLY_INDEX_APPEND_FACE_TYPED((PST), uint32_t,                          \
                                    (FACE), (FACE_COUNT), (OUT_COUNT));       \
        break;                                                                \
      default:                                                                \
        break;                                                                \
    }                                                                         \
  } while (0)

#define PLY_INDEX_APPEND_TRI(PST, A, B, C, OUT_COUNT)                         \
  do {                                                                        \
    switch ((PST)->indexComponentType) {                                      \
      case AKT_UBYTE:                                                         \
        PLY_INDEX_APPEND_TRI_TYPED((PST), uint8_t, (A), (B), (C));            \
        break;                                                                \
      case AKT_USHORT:                                                        \
        PLY_INDEX_APPEND_TRI_TYPED((PST), uint16_t, (A), (B), (C));           \
        break;                                                                \
      case AKT_UINT:                                                          \
        PLY_INDEX_APPEND_TRI_TYPED((PST), uint32_t, (A), (B), (C));           \
        break;                                                                \
      default:                                                                \
        break;                                                                \
    }                                                                         \
    (OUT_COUNT) += 3;                                                         \
  } while (0)

#define PLY_INDEX_APPEND_STRIP_TRI_SEEN(PST, A, B, C, STRIP_LEN, OUT_COUNT,   \
                                        SEEN)                                  \
  do {                                                                        \
    AkUInt strip_a_, strip_b_, strip_c_;                                      \
                                                                              \
    strip_a_ = (A);                                                           \
    strip_b_ = (B);                                                           \
    strip_c_ = (C);                                                           \
    if (strip_a_ != strip_b_                                                   \
        && strip_a_ != strip_c_                                                \
        && strip_b_ != strip_c_                                                \
        && ply_tri_seen_insert((SEEN),                                         \
                               strip_a_,                                       \
                               strip_b_,                                       \
                               strip_c_)) {                                    \
      if (((STRIP_LEN) & 1u) == 0u)                                           \
        PLY_INDEX_APPEND_TRI((PST),                                           \
                             strip_a_,                                        \
                             strip_b_,                                        \
                             strip_c_,                                        \
                             (OUT_COUNT));                                    \
      else                                                                    \
        PLY_INDEX_APPEND_TRI((PST),                                           \
                             strip_b_,                                        \
                             strip_a_,                                        \
                             strip_c_,                                        \
                             (OUT_COUNT));                                    \
    }                                                                         \
  } while (0)

#define PLY_INDEX_APPEND_STRIP_TRI(PST, A, B, C, STRIP_LEN, OUT_COUNT)        \
  PLY_INDEX_APPEND_STRIP_TRI_SEEN((PST),                                       \
                                  (A),                                         \
                                  (B),                                         \
                                  (C),                                         \
                                  (STRIP_LEN),                                 \
                                  (OUT_COUNT),                                 \
                                  NULL)

#define PLY_INDEX_APPEND_TRI_TYPED(PST, TYPE, A, B, C)                        \
  do {                                                                        \
    AkDataContext *dctx_;                                                     \
    AkDataChunk   *chunk_;                                                    \
    AkUInt a_, b_, c_, max_;                                                  \
    TYPE  *dst_;                                                              \
    size_t size_;                                                             \
                                                                              \
    dctx_ = (PST)->dc_ind;                                                    \
    a_    = (A);                                                              \
    b_    = (B);                                                              \
    c_    = (C);                                                              \
    max_  = (PST)->indexMax;                                                  \
    if (a_ > max_) max_ = a_;                                                 \
    if (b_ > max_) max_ = b_;                                                 \
    if (c_ > max_) max_ = c_;                                                 \
    (PST)->indexMax = max_;                                                   \
                                                                              \
    size_ = sizeof(TYPE) * 3u;                                                \
    if (!dctx_->last || dctx_->last->usedsize + size_ > dctx_->nodesize) {     \
      chunk_ = ak_heap_alloc(dctx_->heap,                                     \
                             dctx_,                                           \
                             sizeof(*chunk_) + dctx_->nodesize);              \
      chunk_->usedsize = 0;                                                   \
      chunk_->next     = NULL;                                                \
      if (dctx_->last)                                                        \
        dctx_->last->next = chunk_;                                           \
      dctx_->last  = chunk_;                                                  \
      dctx_->size += dctx_->nodesize;                                         \
      if (!dctx_->data)                                                       \
        dctx_->data = chunk_;                                                 \
    } else {                                                                  \
      chunk_ = dctx_->last;                                                   \
    }                                                                         \
                                                                              \
    dst_    = (TYPE *)(void *)(chunk_->data + chunk_->usedsize);              \
    dst_[0] = (TYPE)a_;                                                       \
    dst_[1] = (TYPE)b_;                                                       \
    dst_[2] = (TYPE)c_;                                                       \
    chunk_->usedsize += size_;                                                \
    dctx_->usedsize  += size_;                                                \
    dctx_->itemcount += 3;                                                    \
  } while (0)

#endif /* ply_util_h */
