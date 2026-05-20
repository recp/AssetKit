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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

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
      case AKT_FLOAT:  DEST = (T)(*(float    *)(void *)&buf);     break;      \
      case AKT_INT:    DEST = (T)(*(int32_t  *)(void *)&buf);     break;      \
      case AKT_UINT:   DEST = (T)(*(uint32_t *)(void *)&buf);     break;      \
      case AKT_DOUBLE: DEST = (T)(*(double   *)(void *)&buf);     break;      \
      case AKT_INT64:  DEST = (T)(*(int64_t  *)(void *)&buf);     break;      \
      case AKT_UINT64: DEST = (T)(*(uint64_t *)(void *)&buf);     break;      \
      case AKT_SHORT:  DEST = (T)(*(int16_t  *)(void *)&buf);     break;      \
      case AKT_USHORT: DEST = (T)(*(uint16_t *)(void *)&buf);     break;      \
      case AKT_BYTE:   DEST = (T)(*(int8_t   *)(void *)&buf);     break;      \
      case AKT_UBYTE:  DEST = (T)(*(uint8_t  *)(void *)&buf);     break;      \
      default:         DEST = DEFAULT;                    break;              \
    }                                                                         \
  } while (0)

#pragma GCC diagnostic pop

static inline
AkDataContext*
ply_index_data_new_for(PLYState * __restrict pst,
                       AkTypeId * __restrict componentType,
                       AkUInt   * __restrict indexMax) {
  AkUInt maxIndex;
  size_t itemSize, nodeItems;

  maxIndex = pst->vertcount > 0 ? pst->vertcount - 1 : 0;
  *componentType = ak_indexComponentTypeForMax(maxIndex);
  *indexMax = 0;
  itemSize = ak_indexComponentSize(*componentType);
  nodeItems = 1024 / itemSize;
  if (nodeItems < 128)
    nodeItems = 128;

  return ak_data_new(pst->tmp, nodeItems, itemSize, NULL);
}

static inline
size_t
ply_tristrip_seen_capacity(size_t count) {
  size_t cap, wanted;

  if (count < 3 || count > SIZE_MAX / 2)
    return 0;

  wanted = (count - 2) * 2;
  cap    = 64;
  while (cap < wanted) {
    if (cap > SIZE_MAX / 2)
      return 0;
    cap <<= 1;
  }

  return cap;
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
bool
ply_tri_seen_insert(PLYTriSeenSlot * __restrict seen,
                    size_t                       seenCap,
                    AkUInt                       a,
                    AkUInt                       b,
                    AkUInt                       c) {
  PLYTriSeenSlot *slot;
  uint32_t        sa, sb, sc;
  size_t          mask, pos;

  if (!seen || seenCap == 0)
    return true;
  if (a > UINT32_MAX || b > UINT32_MAX || c > UINT32_MAX)
    return true;

  sa = (uint32_t)a;
  sb = (uint32_t)b;
  sc = (uint32_t)c;
  ply_tri_sort3(&sa, &sb, &sc);

  mask = seenCap - 1;
  pos  = (size_t)ply_tri_hash(sa, sb, sc) & mask;
  for (;;) {
    slot = &seen[pos];
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
                                        SEEN, SEEN_CAP)                       \
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
                               (SEEN_CAP),                                     \
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
                                  NULL,                                        \
                                  0)

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
