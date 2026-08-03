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

/*
 Resources:
   https://all3dp.com/what-is-stl-file-format-extension-3d-printing/
   https://danbscott.ghost.io/writing-an-stl-file-from-scratch/
   https://en.wikipedia.org/wiki/STL_%28file_format%29
*/

#include "stl.h"
#include "common.h"
#include "../../id.h"
#include "../../data.h"
#include "../../color.h"
#include "../../mat/internal.h"
#include "../../../include/ak/path.h"
#include "../common/util.h"
#include "../common/postscript.h"
#include "../../endian.h"
#include "../../strpool.h"

/* Direct-mapped cache for repeated STL position tuples. 64k is measurably
   better on dense shared-edge triangle soups; allocate it as one scratch block
   in the caller so small-stack platforms do not pay a 1MB frame. */
#define STL_POSITION_CACHE_SIZE 65536u
#define stl_data_append_slot ak_data_append_slot

AK_INLINE
uint16_t
stl_read_u16le(const char * __restrict p) {
  uint16_t v;

  memcpy(&v, p, sizeof(v));
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
  v = bswapu16(v);
#endif

  return v;
}

AK_INLINE
uint32_t
stl_read_u32le(const char * __restrict p) {
  uint32_t v;

  memcpy(&v, p, sizeof(v));
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
  v = bswapu32(v);
#endif

  return v;
}

AK_INLINE
bool
stl_binary_size_valid(const char * __restrict p,
                      size_t                   size,
                      uint32_t * __restrict    nTriangles) {
  uint32_t count;
  uint64_t expected;

  if (size < 84)
    return false;

  count    = stl_read_u32le(p + 80);
  expected = 84ull + (uint64_t)count * 50ull;
  if (expected > (uint64_t)size)
    return false;

  if (nTriangles)
    *nTriangles = count;

  return true;
}

AK_INLINE
bool
stl_starts_solid(const char * __restrict p,
                 size_t                   size) {
  return size >= 5
         && ak_str_pack4_ci_fast(p, 4) == AK_STR_PACK4_CHARS('s','o','l','i')
         && ak_str_ascii_lower_fast(p[4]) == 'd';
}

static
bool
stl_ascii_likely(const char * __restrict p,
                 size_t                   size) {
  size_t i, scan;

  if (!stl_starts_solid(p, size))
    return false;

  scan = size < 512 ? size : 512;
  for (i = 5; i < scan; i++) {
    if (p[i] == '\0')
      return false;

    if (p[i] != '\n' && p[i] != '\r')
      continue;

    i++;
    while (i < scan
           && (p[i] == ' ' || p[i] == '\t'
               || p[i] == '\f' || p[i] == '\v'
               || p[i] == '\n' || p[i] == '\r'))
      i++;

    if (i + 5 <= scan
        && ak_str_pack4_ci_fast(p + i, 4) == AK_STR_PACK4_CHARS('f','a','c','e')
        && ak_str_ascii_lower_fast(p[i + 4]) == 't')
      return true;

    if (i + 8 <= scan
        && ak_str_pack8_ci_fast(p + i, 8)
           == AK_STR_PACK8_CHARS('e','n','d','s','o','l','i','d'))
      return true;
  }

  return false;
}

static
float
stl_color_5bit(uint16_t v) {
  static const float table[32] = {
    0.000000000e+00f, 2.496754220e-03f, 5.370537349e-03f,
    9.529478582e-03f, 1.513350723e-02f, 2.229888301e-02f,
    3.113008991e-02f, 4.172257392e-02f, 5.416457340e-02f,
    6.853841291e-02f, 8.492145665e-02f, 1.033868345e-01f,
    1.240040094e-01f, 1.468392308e-01f, 1.719559031e-01f,
    1.994148902e-01f, 2.292747699e-01f, 2.615920503e-01f,
    2.964213533e-01f, 3.338155745e-01f, 3.738260210e-01f,
    4.165025327e-01f, 4.618935895e-01f, 5.100464057e-01f,
    5.610070144e-01f, 6.148203435e-01f, 6.715302834e-01f,
    7.311797491e-01f, 7.938107351e-01f, 8.594643671e-01f,
    9.281809479e-01f, 1.000000000e+00f
  };

  return table[v];
}

static
void
stl_decode_viscam_color(uint16_t attr, vec4 color) {
  color[0] = stl_color_5bit((uint16_t)((attr >> 10) & 31u));
  color[1] = stl_color_5bit((uint16_t)((attr >> 5)  & 31u));
  color[2] = stl_color_5bit((uint16_t)( attr        & 31u));
  color[3] = 1.0f;
}

static
void
stl_decode_magics_color(uint16_t attr, vec4 color) {
  color[0] = stl_color_5bit((uint16_t)( attr        & 31u));
  color[1] = stl_color_5bit((uint16_t)((attr >> 5)  & 31u));
  color[2] = stl_color_5bit((uint16_t)((attr >> 10) & 31u));
  color[3] = 1.0f;
}

static
bool
stl_header_color(const char * __restrict header, vec4 color) {
  uint32_t i;

  for (i = 0; i + 10 <= 80; i++) {
    if (header[i] == 'C'
        && header[i + 1] == 'O'
        && header[i + 2] == 'L'
        && header[i + 3] == 'O'
        && header[i + 4] == 'R'
        && header[i + 5] == '=') {
      const unsigned char *rgba;

      rgba     = (const unsigned char *)(const void *)(header + i + 6);
      /* Packed STL colors are display-referred in interoperable tools.
         Canonical AssetKit values are linear-sRGB. */
      color[0] = ak_srgb8_to_linearf_fast(rgba[0]);
      color[1] = ak_srgb8_to_linearf_fast(rgba[1]);
      color[2] = ak_srgb8_to_linearf_fast(rgba[2]);
      color[3] = (float)rgba[3] / 255.0f;
      return true;
    }
  }

  return false;
}

typedef struct STLDedup {
  float    *pos;
  float    *nor;
  float    *col;
  uint32_t *table;
  uint32_t *indices;
  size_t    tableCap;
  uint32_t  count;
  uint32_t  indexCount;
  bool      hasColor;
} STLDedup;

typedef struct STLPositionDedup {
  uint32_t *posBits;
  uint32_t *table;
  uint32_t *indices;
  AkIndexArray *indexArray;
  size_t    tableCap;
  size_t    posCap;
  uint32_t  count;
  uint32_t  indexCount;
} STLPositionDedup;

static
void
stl_dedup_free(STLDedup * __restrict dedup) {
  free(dedup->pos);
  free(dedup->nor);
  free(dedup->col);
  free(dedup->table);
  free(dedup->indices);
  memset(dedup, 0, sizeof(*dedup));
}

static
void
stl_position_dedup_free(STLPositionDedup * __restrict dedup) {
  free(dedup->posBits);
  free(dedup->table);
  if (dedup->indexArray)
    ak_free(dedup->indexArray);
  else
    free(dedup->indices);
  memset(dedup, 0, sizeof(*dedup));
}

static
bool
stl_next_hash_cap(size_t count, size_t * __restrict capOut) {
  size_t cap, target;

  if (count > (SIZE_MAX / 2))
    return false;

  target = count < 8 ? 16 : count * 2;
  cap    = 16;
  while (cap < target) {
    if (cap > (SIZE_MAX / 2))
      return false;
    cap *= 2;
  }

  *capOut = cap;
  return true;
}

static
bool
stl_position_dedup_init(STLPositionDedup * __restrict dedup,
                        size_t                         indexCount,
                        size_t                         triangleCount,
                        size_t                         tableCountHint,
                        AkHeap * __restrict            heap,
                        void * __restrict              indexParent,
                        bool                           directUintIndices) {
  size_t tableCap, tableCount;
  size_t posBytes;
  size_t posCap;

  memset(dedup, 0, sizeof(*dedup));
  if (indexCount == 0 || indexCount >= UINT32_MAX)
    return false;

  tableCount = tableCountHint;
  if (tableCount == 0 || tableCount > indexCount) {
    tableCount = indexCount;
  }
  if (tableCountHint == 0 && triangleCount > 0 && indexCount / 3u > 1024u)
    tableCount = indexCount / 3u;

  if (!stl_next_hash_cap(tableCount, &tableCap))
    return false;

  posCap = tableCount < indexCount ? tableCount : indexCount;
  if (posCap == 0 || posCap > SIZE_MAX / (sizeof(*dedup->posBits) * 3))
    return false;
  posBytes = posCap * sizeof(*dedup->posBits) * 3;

  dedup->table    = calloc(tableCap, sizeof(*dedup->table));
  dedup->tableCap = tableCap;
  dedup->posCap   = posCap;
  dedup->posBits  = malloc(posBytes);
  if (directUintIndices && heap && indexParent) {
    dedup->indexArray = ak_indexArrayAlloc(heap,
                                           indexParent,
                                           indexCount,
                                           AKT_UINT);
    if (dedup->indexArray)
      dedup->indices = (uint32_t *)(void *)dedup->indexArray->items;
  } else {
    dedup->indices = malloc(indexCount * sizeof(*dedup->indices));
  }

  if (!dedup->posBits
      || !dedup->indices
      || !dedup->table) {
    stl_position_dedup_free(dedup);
    return false;
  }

  return true;
}

static
bool
stl_dedup_init(STLDedup * __restrict dedup,
               size_t                 indexCount,
               bool                   hasColor) {
  size_t tableCap;

  memset(dedup, 0, sizeof(*dedup));
  if (indexCount == 0 || indexCount >= UINT32_MAX)
    return false;
  if (!stl_next_hash_cap(indexCount, &tableCap))
    return false;
  if (indexCount > SIZE_MAX / (sizeof(float) * 3))
    return false;
  if (hasColor && indexCount > SIZE_MAX / (sizeof(float) * 4))
    return false;

  dedup->pos       = malloc(indexCount * sizeof(float) * 3);
  dedup->nor       = malloc(indexCount * sizeof(float) * 3);
  dedup->col       = hasColor ? malloc(indexCount * sizeof(float) * 4) : NULL;
  dedup->indices   = malloc(indexCount * sizeof(*dedup->indices));
  dedup->table     = calloc(tableCap, sizeof(*dedup->table));
  dedup->tableCap  = tableCap;
  dedup->hasColor  = hasColor;
  dedup->indexCount = (uint32_t)indexCount;

  if (!dedup->pos
      || !dedup->nor
      || (hasColor && !dedup->col)
      || !dedup->indices
      || !dedup->table) {
    stl_dedup_free(dedup);
    return false;
  }

  return true;
}

AK_INLINE
uint32_t
stl_hash3_u32(uint32_t a, uint32_t b, uint32_t c) {
  uint32_t h;

  h = a * 73856093u;
  h ^= b * 19349663u;
  h ^= c * 83492791u;
  h ^= h >> 16;
  return h;
}

static
uint64_t
stl_hash_bytes(uint64_t h, const void * __restrict data, size_t len) {
  const uint8_t *p, *end;

  p   = data;
  end = p + len;
  while (p < end) {
    h ^= *p++;
    h *= 1099511628211ull;
  }

  return h;
}

AK_INLINE
uint32_t
stl_position_cache_slot_u32(uint32_t b0, uint32_t b1, uint32_t b2) {
  return (b0 ^ b1 ^ b2) & (STL_POSITION_CACHE_SIZE - 1u);
}

static
uint64_t
stl_vertex_hash(const float * __restrict pos,
                const float * __restrict nor,
                const float * __restrict col,
                bool                     hasColor) {
  uint64_t h;

  h = 1469598103934665603ull;
  h = stl_hash_bytes(h, pos, sizeof(float) * 3);
  h = stl_hash_bytes(h, nor, sizeof(float) * 3);
  if (hasColor)
    h = stl_hash_bytes(h, col, sizeof(float) * 4);

  return h;
}

static
bool
stl_vertex_equal(const STLDedup * __restrict dedup,
                 uint32_t                    index,
                 const float * __restrict    pos,
                 const float * __restrict    nor,
                 const float * __restrict    col) {
  return memcmp(dedup->pos + (size_t)index * 3,
                pos,
                sizeof(float) * 3) == 0
         && memcmp(dedup->nor + (size_t)index * 3,
                   nor,
                   sizeof(float) * 3) == 0
         && (!dedup->hasColor
             || memcmp(dedup->col + (size_t)index * 4,
                       col,
                       sizeof(float) * 4) == 0);
}

AK_INLINE
bool
stl_position_dedup_intern_u32_h(STLPositionDedup * __restrict dedup,
                                uint32_t                       b0,
                                uint32_t                       b1,
                                uint32_t                       b2,
                                uint32_t                       hash,
                                uint32_t * __restrict          indexOut) {
  size_t   slot, mask;

  mask = dedup->tableCap - 1;
  slot = (size_t)hash & mask;

  for (;;) {
    uint32_t packed, index;

    packed = dedup->table[slot];
    if (packed == 0) {
      if (dedup->count >= dedup->posCap
          || dedup->count + 1u >= dedup->tableCap)
        return false;
      index = dedup->count++;
      dedup->posBits[(size_t)index * 3 + 0] = b0;
      dedup->posBits[(size_t)index * 3 + 1] = b1;
      dedup->posBits[(size_t)index * 3 + 2] = b2;
      dedup->table[slot] = index + 1;
      *indexOut          = index;
      return true;
    }

    index = packed - 1;
    if (dedup->posBits[(size_t)index * 3 + 0] == b0
        && dedup->posBits[(size_t)index * 3 + 1] == b1
        && dedup->posBits[(size_t)index * 3 + 2] == b2) {
      *indexOut = index;
      return true;
    }

    slot = (slot + 1) & mask;
  }
}

AK_INLINE
bool
stl_position_dedup_intern_raw_cached(STLPositionDedup * __restrict dedup,
                                     const char * __restrict       pos,
                                     uint32_t * __restrict         cacheBits,
                                     uint32_t * __restrict         cachePacked,
                                     uint32_t * __restrict         indexOut) {
  uint32_t b0, b1, b2, hash, cacheSlot, packed;

  b0 = stl_read_u32le(pos + 0);
  b1 = stl_read_u32le(pos + 4);
  b2 = stl_read_u32le(pos + 8);

  cacheSlot = stl_position_cache_slot_u32(b0, b1, b2);
  packed = cachePacked[cacheSlot];
  if (packed) {
    const uint32_t *cached = cacheBits + (size_t)cacheSlot * 3;
    if (cached[0] == b0 && cached[1] == b1 && cached[2] == b2) {
      *indexOut = packed - 1u;
      return true;
    }
  }

  hash = stl_hash3_u32(b0, b1, b2);
  if (!stl_position_dedup_intern_u32_h(dedup, b0, b1, b2, hash, indexOut))
    return false;

  cacheBits[(size_t)cacheSlot * 3 + 0] = b0;
  cacheBits[(size_t)cacheSlot * 3 + 1] = b1;
  cacheBits[(size_t)cacheSlot * 3 + 2] = b2;
  cachePacked[cacheSlot] = *indexOut + 1u;
  return true;
}

static
bool
stl_dedup_intern(STLDedup * __restrict dedup,
                 const float * __restrict pos,
                 const float * __restrict nor,
                 const float * __restrict col,
                 uint32_t * __restrict    indexOut) {
  uint64_t hash;
  size_t   slot, mask;

  hash = stl_vertex_hash(pos, nor, col, dedup->hasColor);
  mask = dedup->tableCap - 1;
  slot = (size_t)hash & mask;

  for (;;) {
    uint32_t packed, index;

    packed = dedup->table[slot];
    if (packed == 0) {
      index = dedup->count++;
      memcpy(dedup->pos + (size_t)index * 3, pos, sizeof(float) * 3);
      memcpy(dedup->nor + (size_t)index * 3, nor, sizeof(float) * 3);
      if (dedup->hasColor)
        memcpy(dedup->col + (size_t)index * 4, col, sizeof(float) * 4);
      dedup->table[slot] = index + 1;
      *indexOut          = index;
      return true;
    }

    index = packed - 1;
    if (stl_vertex_equal(dedup, index, pos, nor, col)) {
      *indexOut = index;
      return true;
    }

    slot = (slot + 1) & mask;
  }
}

static
AkBuffer*
stl_buffer_alloc(AkHeap * __restrict heap,
                 AkDoc  * __restrict doc,
                 size_t              length) {
  AkBuffer *buff;

  buff         = ak_heap_calloc(heap, doc, sizeof(*buff));
  buff->length = length;
  buff->data   = length ? ak_heap_alloc(heap, buff, length) : NULL;
  AK_LIB_PREPEND(doc->lib.buffers, buff, next);

  return buff;
}

static
bool
stl_position_dedup_emit(STLState * __restrict sst,
                        STLPositionDedup * __restrict dedup) {
  AkBuffer *posBuff;
  size_t    count;

  count = dedup->count;
  if (count == 0 || count > UINT32_MAX || dedup->indexCount == 0)
    return false;

  posBuff = stl_buffer_alloc(sst->heap,
                             sst->doc,
                             sizeof(float) * 3 * count);
  if (!posBuff || !posBuff->data)
    return false;

  memcpy(posBuff->data, dedup->posBits, posBuff->length);

  sst->buff_pos  = posBuff;
  sst->count     = (uint32_t)count;
  sst->maxVC     = 3;
  sst->raw_indices     = dedup->indices;
  sst->raw_index_array = dedup->indexArray;
  sst->indexCount      = dedup->indexCount;
  sst->indexMax        = dedup->count - 1;
  if (dedup->indexArray) {
    dedup->indexArray->count = dedup->indexCount;
    dedup->indexArray->max   = sst->indexMax;
  }
  dedup->indices    = NULL;
  dedup->indexArray = NULL;

  return true;
}

static
bool
stl_dedup_emit(STLState * __restrict sst,
               STLDedup * __restrict dedup) {
  AkBuffer *posBuff, *norBuff, *colBuff;
  size_t    count;

  count = dedup->count;
  if (count == 0 || count > UINT32_MAX)
    return false;

  posBuff = stl_buffer_alloc(sst->heap,
                             sst->doc,
                             sizeof(float) * 3 * count);
  norBuff = stl_buffer_alloc(sst->heap,
                             sst->doc,
                             sizeof(float) * 3 * count);
  if (!posBuff || !norBuff || !posBuff->data || !norBuff->data)
    return false;

  memcpy(posBuff->data, dedup->pos, posBuff->length);
  memcpy(norBuff->data, dedup->nor, norBuff->length);

  sst->buff_pos = posBuff;
  sst->buff_nor = norBuff;
  sst->count    = (uint32_t)count;

  if (dedup->hasColor) {
    colBuff = stl_buffer_alloc(sst->heap,
                               sst->doc,
                               sizeof(float) * 4 * count);
    if (!colBuff || !colBuff->data)
      return false;
    memcpy(colBuff->data, dedup->col, colBuff->length);
    sst->buff_col = colBuff;
  }

  if (dedup->count < dedup->indexCount) {
    sst->raw_indices = dedup->indices;
    sst->indexCount  = dedup->indexCount;
    sst->indexMax    = dedup->count - 1;
    dedup->indices   = NULL;
  }

  return true;
}

static
bool
stl_dedup_from_arrays(STLState * __restrict sst,
                      const float * __restrict pos,
                      const float * __restrict nor,
                      const float * __restrict col,
                      size_t                    indexCount,
                      bool                      hasColor) {
  STLDedup dedup;
  size_t   i;
  bool     ok;

  if (!stl_dedup_init(&dedup, indexCount, hasColor))
    return false;

  ok = true;
  for (i = 0; i < indexCount; i++) {
    uint32_t index;

    if (!stl_dedup_intern(&dedup,
                          pos + i * 3,
                          nor + i * 3,
                          hasColor ? col + i * 4 : NULL,
                          &index)) {
      ok = false;
      break;
    }
    dedup.indices[i] = index;
  }

  if (ok)
    ok = stl_dedup_emit(sst, &dedup);

  stl_dedup_free(&dedup);
  return ok;
}

AK_INLINE
char*
stl_skip_inline_space(char * __restrict p) {
  while (p[0] == ' ' || p[0] == '\t' || p[0] == '\f' || p[0] == '\v')
    p++;
  return p;
}

AK_INLINE
char*
stl_parse_float_token(char * __restrict p,
                      float * __restrict dest) {
  p = stl_skip_inline_space(p);
  return ak_str_parse_float_fast(p, NULL, dest);
}

#define STL_PARSE_FLOAT3(PTR, DEST)                                           \
  do {                                                                        \
    (PTR) = stl_parse_float_token((PTR), &(DEST)[0]);                         \
    (PTR) = stl_parse_float_token((PTR), &(DEST)[1]);                         \
    (PTR) = stl_parse_float_token((PTR), &(DEST)[2]);                         \
  } while (0)

AK_HIDE
AkResult
stl_stl(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap   *heap;
  AkDoc    *doc;
  void     *stlstr;
  char     *p;
  AkScene  *scene;
  AkNode   *rootNode;
  STLState  sstVal = {0}, *sst;
  size_t    stlstrSize;
  bool      isAscii;

  if (ak_readfile(filepath, NULL, &stlstr, &stlstrSize) != AK_OK
      || !((p = stlstr) && stlstrSize > 0)) {
    if (stlstr)
      ak_releasefile(stlstr, stlstrSize);
    return AK_ERR;
  }

  if (stl_ascii_likely(p, stlstrSize)) {
    isAscii = true;
  } else if (stl_binary_size_valid(p, stlstrSize, NULL)) {
    isAscii = false;
  } else if (stl_starts_solid(p, stlstrSize)) {
    isAscii = true;
  } else {
    ak_releasefile(stlstr, stlstrSize);
    return AK_ERR;
  }
  
  heap  = ak_heap_new(NULL, NULL, NULL);
  doc   = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf                = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name          = ak_heap_strdup(heap, doc->inf, filepath);
  doc->inf->dir           = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage     = true;
  doc->inf->ftype         = AK_FILE_TYPE_STL;
  doc->inf->base.coordSys = AK_YUP;
  doc->coordSys           = AK_YUP; /* Default */
  
  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  /* default scene */
  scene                  = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->node            = ak_heap_calloc(heap, scene, sizeof(*scene->node));
  ak_setypeid(scene->node, AKT_NODE);
  scene->node->visible   = true;
  rootNode               = ak_heap_calloc(heap, doc, sizeof(*rootNode));
  ak_setypeid(rootNode, AKT_NODE);
  rootNode->visible      = true;
  AK_LIB_PREPEND(doc->lib.nodes, rootNode, docNext);
  ak_addSubNode(scene->node, rootNode, false);
  AK_LIB_PREPEND(doc->lib.scenes, scene, next);
  doc->scene             = scene;

  /* parse state */
  memset(&sstVal, 0, sizeof(sstVal));
  sst              = &sstVal;
  sstVal.doc       = doc;
  sstVal.heap      = heap;
  sstVal.tmp       = ak_heap_alloc(heap, doc, sizeof(void*));
  sstVal.node      = rootNode;
  sstVal.lib_geom  = &doc->lib.geometries;
  
  if (!isAscii) {
    stl_binary(sst, p);
  } else {
    sst->dc_pos    = ak_data_new(sst->tmp,
                                 STL_DATA_NODE_ITEMS,
                                 sizeof(vec3),
                                 NULL);
    sst->dc_nor    = ak_data_new(sst->tmp,
                                 STL_DATA_NODE_ITEMS,
                                 sizeof(vec3),
                                 NULL);
    sst->dc_vcount = ak_data_new(sst->tmp,
                                 STL_DATA_NODE_ITEMS,
                                 sizeof(int32_t),
                                 NULL);
    stl_ascii(sst, p);
  }
  
  sst_finish(sst);
  io_postscript(doc);
  
  *dest = doc;

  /* cleanup */
  ak_free(sst->tmp);
  ak_releasefile(stlstr, stlstrSize);

  return AK_OK;
}

static
bool
stl_binary_fill_dedup(STLDedup * __restrict dedup,
                      char * __restrict     p,
                      uint32_t              nTriangles,
                      bool                  hasHeaderColor,
                      bool                  hasColors,
                      vec4                  defaultColor) {
  size_t   outIndex;
  uint32_t i;
  bool     ok;

  outIndex = 0;
  ok       = true;
  for (i = 0; i < nTriangles; i++) {
    vec3     normal, vertices[3];
    vec4     color;
    uint16_t attr;
    uint32_t v;

    le_32(normal[0], p);
    le_32(normal[1], p);
    le_32(normal[2], p);

    le_32(vertices[0][0], p);
    le_32(vertices[0][1], p);
    le_32(vertices[0][2], p);

    le_32(vertices[1][0], p);
    le_32(vertices[1][1], p);
    le_32(vertices[1][2], p);

    le_32(vertices[2][0], p);
    le_32(vertices[2][1], p);
    le_32(vertices[2][2], p);

    attr = stl_read_u16le(p);
    if (hasColors) {
      if (hasHeaderColor) {
        if ((attr & 0x8000u) == 0 && (attr & 0x7fffu) != 0)
          stl_decode_magics_color(attr, color);
        else
          memcpy(color, defaultColor, sizeof(color));
      } else if ((attr & 0x8000u) != 0) {
        stl_decode_viscam_color(attr, color);
      } else {
        color[0] = 1.0f;
        color[1] = 1.0f;
        color[2] = 1.0f;
        color[3] = 1.0f;
      }
    }

    for (v = 0; v < 3; v++) {
      uint32_t index;

      if (!stl_dedup_intern(dedup,
                            vertices[v],
                            normal,
                            hasColors ? color : NULL,
                            &index)) {
        ok = false;
        break;
      }
      dedup->indices[outIndex++] = index;
    }
    if (!ok)
      break;

    p += 2;
  }

  return ok;
}

static
bool
stl_binary_fill_position_dedup(STLPositionDedup * __restrict dedup,
                               char * __restrict             p,
                               uint32_t                      nTriangles,
                               uint32_t * __restrict         cacheBits,
                               uint32_t * __restrict         cachePacked) {
  uint32_t i;
  uint32_t colorMask;

  memset(cachePacked, 0, STL_POSITION_CACHE_SIZE * sizeof(*cachePacked));
  colorMask = 0;
  for (i = 0; i < nTriangles; i++) {
    uint32_t i0, i1, i2;

    p += 12; /* facet normal */

    if (!stl_position_dedup_intern_raw_cached(dedup,
                                              p,
                                              cacheBits,
                                              cachePacked,
                                              &i0))
      return false;
    p += 12;
    if (!stl_position_dedup_intern_raw_cached(dedup,
                                              p,
                                              cacheBits,
                                              cachePacked,
                                              &i1))
      return false;
    p += 12;
    if (!stl_position_dedup_intern_raw_cached(dedup,
                                              p,
                                              cacheBits,
                                              cachePacked,
                                              &i2))
      return false;
    p += 12;

    colorMask |= (uint32_t)stl_read_u16le(p) & 0x8000u;
    p += 2; /* attribute byte count */

    if (i0 == i1 || i0 == i2 || i1 == i2)
      continue;

    dedup->indices[dedup->indexCount++] = i0;
    dedup->indices[dedup->indexCount++] = i1;
    dedup->indices[dedup->indexCount++] = i2;
  }

  return colorMask == 0;
}

static
bool
stl_binary_position_dedup(STLState * __restrict sst, char * __restrict p) {
  STLPositionDedup dedup;
  char     *header, *body;
  uint32_t  nTriangles, sampleTriangles;
  size_t    indexCount, tableCountHint;
  bool      ok;
  bool      directUintIndices;
  vec4      defaultColor;
  uint32_t *cacheBits;
  uint32_t *cachePacked;

  if (!ak_opt_get(AK_OPT_MESH_POSITION_DEDUP_INDEX))
    return false;

  header = p;
  p += 80;
  le_32(nTriangles, p);
  body = p;

  if (nTriangles == 0 || nTriangles > UINT32_MAX / 3u)
    return false;
  if (stl_header_color(header, defaultColor))
    return false;

  cacheBits = malloc(STL_POSITION_CACHE_SIZE * 4u * sizeof(*cacheBits));
  if (!cacheBits)
    return false;
  cachePacked = cacheBits + STL_POSITION_CACHE_SIZE * 3u;

  indexCount = (size_t)nTriangles * 3u;
  tableCountHint = 0;
  sampleTriangles = nTriangles > 4096u ? 4096u : nTriangles;
  if (sampleTriangles < nTriangles && sampleTriangles > 0) {
    STLPositionDedup sample;
    size_t sampleIndexCount;

    sampleIndexCount = (size_t)sampleTriangles * 3u;
    if (!stl_position_dedup_init(&sample,
                                 sampleIndexCount,
                                 sampleTriangles,
                                 0,
                                 NULL,
                                 NULL,
                                 false)) {
      free(cacheBits);
      return false;
    }
    ok = stl_binary_fill_position_dedup(&sample,
                                        body,
                                        sampleTriangles,
                                        cacheBits,
                                        cachePacked);
    if (!ok || sample.indexCount == 0
        || (uint64_t)sample.count * 100ull
             > (uint64_t)sample.indexCount * 95ull) {
      stl_position_dedup_free(&sample);
      free(cacheBits);
      return false;
    }
    tableCountHint = (size_t)(((uint64_t)sample.count
                               * (uint64_t)indexCount
                               + (uint64_t)sample.indexCount - 1ull)
                              / (uint64_t)sample.indexCount);
    tableCountHint += tableCountHint / 4u + 1024u;
    if (tableCountHint > indexCount)
      tableCountHint = indexCount;
    stl_position_dedup_free(&sample);
  }

  directUintIndices = tableCountHint > UINT16_MAX;
  if (!stl_position_dedup_init(&dedup,
                               indexCount,
                               nTriangles,
                               tableCountHint,
                               sst->heap,
                               sst->doc,
                               directUintIndices)) {
    free(cacheBits);
    return false;
  }

  ok = stl_binary_fill_position_dedup(&dedup,
                                      body,
                                      nTriangles,
                                      cacheBits,
                                      cachePacked);
  if (ok)
    ok = dedup.indexCount > 0
         && dedup.count < dedup.indexCount
         && stl_position_dedup_emit(sst, &dedup);

  stl_position_dedup_free(&dedup);
  free(cacheBits);
  return ok;
}

static
bool
stl_binary_dedup(STLState * __restrict sst, char * __restrict p) {
  STLDedup dedup;
  char     *header, *body, *scan;
  vec4      defaultColor;
  uint32_t  nTriangles, sampleTriangles, i;
  size_t    indexCount;
  bool      hasHeaderColor, hasFacetColor, hasColors, ok;

  header = p;
  p     += 80;
  le_32(nTriangles, p);
  body   = p;

  if (nTriangles > UINT32_MAX / 3u)
    return false;

  hasHeaderColor = stl_header_color(header, defaultColor);
  sampleTriangles = nTriangles > 4096u ? 4096u : nTriangles;

  if (hasHeaderColor && defaultColor[3] < 0.999f)
    sst->hasColorAlpha = true;

  if (sampleTriangles < nTriangles && sampleTriangles > 0) {
    STLDedup sample;

    if (!stl_dedup_init(&sample, (size_t)sampleTriangles * 3u, false))
      return false;
    ok = stl_binary_fill_dedup(&sample,
                               body,
                               sampleTriangles,
                               false,
                               false,
                               defaultColor);
    if (!ok || (uint64_t)sample.count * 100ull
               > (uint64_t)sample.indexCount * 95ull) {
      stl_dedup_free(&sample);
      return false;
    }
    stl_dedup_free(&sample);
  }

  hasFacetColor = false;
  scan          = body;
  for (i = 0; i < nTriangles; i++) {
    uint16_t attr;

    attr = stl_read_u16le(scan + 48);
    if ((hasHeaderColor && (attr & 0x8000u) == 0 && (attr & 0x7fffu) != 0)
        || (!hasHeaderColor && (attr & 0x8000u) != 0)) {
      hasFacetColor = true;
      break;
    }

    scan += 50;
  }

  hasColors = hasHeaderColor || hasFacetColor;

  indexCount = (size_t)nTriangles * 3u;
  if (!stl_dedup_init(&dedup, indexCount, hasColors))
    return false;

  ok = stl_binary_fill_dedup(&dedup,
                             body,
                             nTriangles,
                             hasHeaderColor,
                             hasColors,
                             defaultColor);

  sst->maxVC = 3;
  if (ok)
    ok = stl_dedup_emit(sst, &dedup);

  stl_dedup_free(&dedup);
  return ok;
}

AK_HIDE
void
stl_binary(STLState * __restrict sst, char * __restrict p) {
  AkBuffer *posBuff, *norBuff, *colBuff;
  float    *pos, *nor, *col;
  char     *header, *scan;
  vec4      defaultColor;
  uint32_t  count,  nTriangles, i;
  bool      hasHeaderColor, hasFacetColor, hasColors;

  if (stl_binary_position_dedup(sst, p))
    return;

  if (stl_binary_dedup(sst, p))
    return;
  
  header = p;

  /* skip 80-char header */
  p += 80;

  /* parse integers from little endian to native */
  le_32(nTriangles, p);

  count      = nTriangles * 3;
  sst->maxVC = 3;
  sst->count = count;

  hasHeaderColor = stl_header_color(header, defaultColor);
  hasFacetColor  = false;
  scan           = p;

  if (hasHeaderColor && defaultColor[3] < 0.999f)
    sst->hasColorAlpha = true;

  for (i = 0; i < nTriangles; i++) {
    uint16_t attr;

    attr = stl_read_u16le(scan + 48);
    if ((hasHeaderColor && (attr & 0x8000u) == 0 && (attr & 0x7fffu) != 0)
        || (!hasHeaderColor && (attr & 0x8000u) != 0)) {
      hasFacetColor = true;
      break;
    }

    scan += 50;
  }

  hasColors = hasHeaderColor || hasFacetColor;

  posBuff         = ak_heap_calloc(sst->heap, sst->doc, sizeof(*posBuff));
  posBuff->length = sizeof(vec3) * count;
  posBuff->data   = ak_heap_alloc(sst->heap, posBuff, posBuff->length);
  AK_LIB_PREPEND(sst->doc->lib.buffers, posBuff, next);

  norBuff         = ak_heap_calloc(sst->heap, sst->doc, sizeof(*norBuff));
  norBuff->length = sizeof(vec3) * count;
  norBuff->data   = ak_heap_alloc(sst->heap, norBuff, norBuff->length);
  AK_LIB_PREPEND(sst->doc->lib.buffers, norBuff, next);

  sst->buff_pos = posBuff;
  sst->buff_nor = norBuff;
  pos = posBuff->data;
  nor = norBuff->data;

  colBuff = NULL;
  col     = NULL;
  if (hasColors) {
    colBuff         = ak_heap_calloc(sst->heap, sst->doc, sizeof(*colBuff));
    colBuff->length = sizeof(vec4) * count;
    colBuff->data   = ak_heap_alloc(sst->heap, colBuff, colBuff->length);
    AK_LIB_PREPEND(sst->doc->lib.buffers, colBuff, next);

    sst->buff_col = colBuff;
    col           = colBuff->data;
  }

  for (i = 0; i < nTriangles; i++) {
    uint16_t attr;

    /* normal */
    le_32(nor[0], p);
    le_32(nor[1], p);
    le_32(nor[2], p);
    memcpy(nor + 3, nor, sizeof(vec3));
    memcpy(nor + 6, nor, sizeof(vec3));
    nor += 9;
    
    /* vertex */
    le_32(pos[0], p);
    le_32(pos[1], p);
    le_32(pos[2], p);
    
    le_32(pos[3], p);
    le_32(pos[4], p);
    le_32(pos[5], p);
    
    le_32(pos[6], p);
    le_32(pos[7], p);
    le_32(pos[8], p);
    pos += 9;

    attr = stl_read_u16le(p);
    if (col) {
      vec4 color;

      if (hasHeaderColor) {
        if ((attr & 0x8000u) == 0 && (attr & 0x7fffu) != 0)
          stl_decode_magics_color(attr, color);
        else
          memcpy(color, defaultColor, sizeof(color));
      } else if ((attr & 0x8000u) != 0) {
        stl_decode_viscam_color(attr, color);
      } else {
        color[0] = 1.0f;
        color[1] = 1.0f;
        color[2] = 1.0f;
        color[3] = 1.0f;
      }

      memcpy(col, color, sizeof(color));
      memcpy(col + 4, color, sizeof(color));
      memcpy(col + 8, color, sizeof(color));
      col += 12;
    }

    p += 2;
  }
}

AK_HIDE
void
stl_ascii(STLState * __restrict sst, char * __restrict p) {
  vec3     n;
  uint32_t vc, count;
  char     c;
  bool     inFacet;

  n[0]    = 0.0f;
  n[1]    = 0.0f;
  n[2]    = 0.0f;
  vc      = 0;
  count   = 0;
  inFacet = false;

  NEXT_LINE

  /* parse ASCII STL */
  do {
    /* skip spaces */
    SKIP_SPACES

    if (STL_EQ5('f', 'a', 'c', 'e', 't')) {
      p += 6;

      SKIP_SPACES

      n[0] = 0.0f;
      n[1] = 0.0f;
      n[2] = 0.0f;
      if (STL_EQ6('n', 'o', 'r', 'm', 'a', 'l')) {
        p += 7;
        STL_PARSE_FLOAT3(p, n);
      }

      vc      = 0;
      inFacet = true;
    } else if (inFacet && STL_EQ6('v', 'e', 'r', 't', 'e', 'x')) {
      float *nor, *pos;

      p += 7;
      pos = stl_data_append_slot(sst->dc_pos);
      STL_PARSE_FLOAT3(p, pos);

      nor    = stl_data_append_slot(sst->dc_nor);
      nor[0] = n[0];
      nor[1] = n[1];
      nor[2] = n[2];

      vc++;
    } else if (inFacet && STL_EQT8('e', 'n', 'd', 'f', 'a', 'c', 'e', 't')) {
      if (vc > 0) {
        count += vc;
        sst->maxVC = GLM_MAX(sst->maxVC, vc);
        *(int32_t *)stl_data_append_slot(sst->dc_vcount) = (int32_t)vc;
      }
      vc      = 0;
      inFacet = false;
    }

    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  if (inFacet && vc > 0) {
    count += vc;
    sst->maxVC = GLM_MAX(sst->maxVC, vc);
    *(int32_t *)stl_data_append_slot(sst->dc_vcount) = (int32_t)vc;
  }
  
  sst->count = count;
}

static
bool
stl_ascii_dedup_triangles(STLState * __restrict sst) {
  float *pos, *nor;
  size_t count, posCount, norCount;
  bool   ok;

  if (!sst->dc_pos || !sst->dc_nor || sst->maxVC != 3)
    return false;

  count = sst->dc_pos->itemcount;
  if (count == 0
      || count > UINT32_MAX
      || sst->dc_nor->itemcount != count
      || count > SIZE_MAX / (sizeof(float) * 3))
    return false;

  pos = malloc(count * sizeof(float) * 3);
  nor = malloc(count * sizeof(float) * 3);
  if (!pos || !nor) {
    free(pos);
    free(nor);
    return false;
  }

  posCount = ak_data_join(sst->dc_pos, pos, 0, 0);
  norCount = ak_data_join(sst->dc_nor, nor, 0, 0);
  ok       = posCount == count
             && norCount == count
             && stl_dedup_from_arrays(sst, pos, nor, NULL, count, false);

  free(pos);
  free(nor);
  return ok;
}

AK_HIDE
void
sst_finish(STLState * __restrict sst) {
  AkHeap             *heap;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;

  /* Buffer > Accessor > Input > Prim > Mesh > Geom > InstanceGeom > Node */
  
  heap = sst->heap;
  mesh = ak_allocMeshEx(sst->heap, sst->doc, &geom, true);

  if (sst->maxVC == 3) {
    AkTriangles *tri;
    
    tri = ak_heap_calloc(sst->heap, ak_objFrom(mesh), sizeof(*tri));
    tri->mode      = AK_TRIANGLES;
    tri->base.type = AK_PRIMITIVE_TRIANGLES;
    prim = (AkMeshPrimitive *)tri;
  } else {
    AkPolygon *poly;
    
    poly = ak_heap_calloc(sst->heap, ak_objFrom(mesh), sizeof(*poly));
    poly->base.type = AK_PRIMITIVE_POLYGONS;
    
    poly->vcount = ak_heap_calloc(heap,
                                  poly,
                                  sizeof(*poly->vcount)
                                  + sst->dc_vcount->usedsize);
    poly->vcount->count = sst->dc_vcount->itemcount;
    ak_data_join(sst->dc_vcount, poly->vcount->items, 0, 0);
    
    prim = (AkMeshPrimitive *)poly;
  }
  
  prim->nPolygons      = sst->maxVC == 3 ? sst->count / 3 : sst->count;
  prim->mesh           = mesh;
  mesh->primitive      = prim;
  mesh->primitiveCount = 1;

  /* add to library */
  AK_LIB_PREPEND(*sst->lib_geom, geom, next);
  
  /* make instance geeometry and attach to the root node  */
  (void)ak_nodeAttachGeometry(sst->node, geom);

  if (!sst->buff_pos && sst->maxVC == 3)
    stl_ascii_dedup_triangles(sst);

  if (sst->raw_indices && sst->indexCount > 0) {
    AkIndexArray *indices;
    AkTypeId      componentType;
    uint32_t      i;

    componentType = ak_indexComponentTypeForMax(sst->indexMax);
    if (componentType == AKT_UINT && sst->raw_index_array) {
      indices                = sst->raw_index_array;
      indices->count         = sst->indexCount;
      indices->max           = sst->indexMax;
      indices->componentType = AKT_UINT;
      ak_heap_setpm(indices, prim);
      sst->raw_index_array   = NULL;
      sst->raw_indices       = NULL;
    } else {
      indices = ak_indexArrayAlloc(heap,
                                   prim,
                                   sst->indexCount,
                                   componentType);
    }
    if (indices) {
      if (sst->raw_indices) {
        switch (componentType) {
          case AKT_UBYTE: {
            uint8_t *dst;

            dst = (uint8_t *)indices->items;
            for (i = 0; i < sst->indexCount; i++)
              dst[i] = (uint8_t)sst->raw_indices[i];
            break;
          }
          case AKT_USHORT: {
            uint16_t *dst;

            dst = (uint16_t *)(void *)indices->items;
            for (i = 0; i < sst->indexCount; i++)
              dst[i] = (uint16_t)sst->raw_indices[i];
            break;
          }
          case AKT_UINT: {
            memcpy(indices->items,
                   sst->raw_indices,
                   sizeof(*sst->raw_indices) * (size_t)sst->indexCount);
            break;
          }
          default:
            break;
        }
      }

      indices->max     = sst->indexMax;
      prim->indices    = indices;
      prim->indexStride = 1;
      if (sst->maxVC == 3)
        prim->nPolygons = sst->indexCount / 3;
    }
  }
  
  if (sst->buff_pos) {
    AkAccessor *acc;

    acc = io_acc(heap,
                 sst->doc,
                 AK_COMPONENT_SIZE_VEC3,
                 AKT_FLOAT,
                 sst->count,
                 sst->buff_pos);
    prim->pos = io_input(heap, prim, acc, AK_INPUT_POSITION, _s_POSITION, 0);

    if (sst->buff_nor) {
      acc = io_acc(heap,
                   sst->doc,
                   AK_COMPONENT_SIZE_VEC3,
                   AKT_FLOAT,
                   sst->count,
                   sst->buff_nor);
      io_input(heap, prim, acc, AK_INPUT_NORMAL, _s_NORMAL, 0);
    }

    if (sst->buff_col) {
      acc = io_acc(heap,
                   sst->doc,
                   AK_COMPONENT_SIZE_VEC4,
                   AKT_FLOAT,
                   sst->count,
                   sst->buff_col);
      io_input(heap, prim, acc, AK_INPUT_COLOR, _s_COLOR, 0);
      prim->material = ak_materialDefaultVertexColorAlpha(sst->doc, sst->hasColorAlpha);
    }
  } else {
    prim->pos = io_addInput(heap, sst->dc_pos, prim, AK_INPUT_POSITION,
                            _s_POSITION, AK_COMPONENT_SIZE_VEC3, AKT_FLOAT, 0);

    if (sst->dc_nor->itemcount > 0) {
      io_addInput(heap, sst->dc_nor, prim, AK_INPUT_NORMAL,
                  _s_NORMAL, AK_COMPONENT_SIZE_VEC3, AKT_FLOAT, 1);
    }
  }

  /* cleanup */
  if (sst->dc_pos) {
    ak_free(sst->dc_pos);
    ak_free(sst->dc_nor);
    ak_free(sst->dc_vcount);
  }
  
  sst->dc_ind    = NULL;
  sst->dc_pos    = NULL;
  sst->dc_nor    = NULL;
  sst->dc_vcount = NULL;
  if (sst->raw_index_array)
    ak_free(sst->raw_index_array);
  else
    free(sst->raw_indices);
  sst->raw_index_array = NULL;
  sst->raw_indices = NULL;
}

#undef STL_PARSE_FLOAT3
