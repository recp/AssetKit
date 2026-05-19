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
#include "../../../include/ak/path.h"
#include "../common/util.h"
#include "../common/postscript.h"
#include "../../endian.h"
#include "../../strpool.h"

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
  return (float)v / 31.0f;
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
      color[0] = (float)rgba[0] / 255.0f;
      color[1] = (float)rgba[1] / 255.0f;
      color[2] = (float)rgba[2] / 255.0f;
      color[3] = (float)rgba[3] / 255.0f;
      return true;
    }
  }

  return false;
}

AK_INLINE
void*
stl_data_append_slot(AkDataContext * __restrict dctx) {
  AkDataChunk *chunk;
  size_t       size;

  size = dctx->itemsize;
  if (dctx->usedsize + size > dctx->size) {
    chunk = ak_heap_alloc(dctx->heap,
                          dctx,
                          sizeof(*chunk) + dctx->nodesize);
    chunk->usedsize = 0;
    chunk->next     = NULL;

    if (dctx->last)
      dctx->last->next = chunk;

    dctx->last  = chunk;
    dctx->size += dctx->nodesize;

    if (!dctx->data)
      dctx->data = chunk;
  } else {
    chunk = dctx->last;
  }

  dctx->usedsize += size;
  dctx->itemcount++;
  chunk->usedsize += size;

  return chunk->data + chunk->usedsize - size;
}

#define STL_PARSE_FLOAT3(PTR, DEST)                                           \
  do {                                                                        \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[0]);                            \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[1]);                            \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[2]);                            \
  } while (0)

AK_HIDE
AkResult
stl_stl(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *stlstr;
  char          *p;
  AkLibrary     *lib_vscene;
  AkVisualScene *scene;
  STLState       sstVal = {0}, *sst;
  size_t         stlstrSize;
  bool           isAscii;

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
  doc->inf->name          = filepath;
  doc->inf->dir           = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage     = true;
  doc->inf->ftype         = AK_FILE_TYPE_STL;
  doc->inf->base.coordSys = AK_YUP;
  doc->coordSys           = AK_YUP; /* Default */
  
  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  /* libraries */
  doc->lib.geometries = ak_heap_calloc(heap, doc, sizeof(AkGeometry));
  lib_vscene = ak_heap_calloc(heap, doc, sizeof(*lib_vscene));
  
  /* default scene */
  scene                  = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->node            = ak_heap_calloc(heap, doc, sizeof(*scene->node));
  lib_vscene->chld       = &scene->base;
  lib_vscene->count      = 1;
  doc->lib.visualScenes  = lib_vscene;
  doc->scene.visualScene = ak_instanceMake(heap, doc, scene);

  /* parse state */
  memset(&sstVal, 0, sizeof(sstVal));
  sst              = &sstVal;
  sstVal.doc       = doc;
  sstVal.heap      = heap;
  sstVal.tmp       = ak_heap_alloc(heap, doc, sizeof(void*));
  sstVal.node      = scene->node;
  sstVal.lib_geom  = doc->lib.geometries;
  
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

AK_HIDE
void
stl_binary(STLState * __restrict sst, char * __restrict p) {
  AkBuffer *posBuff, *norBuff, *colBuff;
  float    *pos, *nor, *col;
  char     *header, *scan;
  vec4      defaultColor;
  uint32_t count,  nTriangles, i;
  bool     hasHeaderColor, hasFacetColor, hasColors;
  
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
  flist_sp_insert(&sst->doc->lib.buffers, posBuff);

  norBuff         = ak_heap_calloc(sst->heap, sst->doc, sizeof(*norBuff));
  norBuff->length = sizeof(vec3) * count;
  norBuff->data   = ak_heap_alloc(sst->heap, norBuff, norBuff->length);
  flist_sp_insert(&sst->doc->lib.buffers, norBuff);

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
    flist_sp_insert(&sst->doc->lib.buffers, colBuff);

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

AK_HIDE
void
sst_finish(STLState * __restrict sst) {
  AkHeap             *heap;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkInstanceGeometry *instGeom;

  /* Buffer > Accessor > Input > Prim > Mesh > Geom > InstanceGeom > Node */
  
  heap = sst->heap;
  mesh = ak_allocMesh(sst->heap, sst->lib_geom, &geom);

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
  geom->base.next      = sst->lib_geom->chld;
  sst->lib_geom->chld  = &geom->base;
  sst->lib_geom->count = 1;
  
  /* make instance geeometry and attach to the root node  */
  instGeom = ak_instanceMakeGeom(heap, sst->node, geom);
  if (sst->node->geometry) {
    sst->node->geometry->base.prev = (void *)instGeom;
    instGeom->base.next            = (void *)sst->node->geometry;
  }

  sst->node->geometry = instGeom;
  
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
}

#undef STL_PARSE_FLOAT3
