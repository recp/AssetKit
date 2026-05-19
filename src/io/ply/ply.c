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
   http://people.math.sc.edu/Burkardt/data/ply/ply.txt
   http://paulbourke.net/dataformats/ply/
   https://en.wikipedia.org/wiki/PLY_(file_format)
   http://gamma.cs.unc.edu/POWERPLANT/papers/ply.pdf
   https://people.sc.fsu.edu/~jburkardt/data/ply/ply.html            (samples)
*/

#include "ply.h"
#include "common.h"
#include "util.h"
#include "../../id.h"
#include "../../data.h"
#include "../../../include/ak/path.h"
#include "../common/util.h"
#include "../common/postscript.h"
#include "../../string_fast.h"
#include "../../strpool.h"

#define PLY_COPY_INDICES(DSTTYPE, SRCTYPE)                                    \
  do {                                                                        \
    AkDataChunk  *chunk_;                                                     \
    const SRCTYPE *src_;                                                      \
    DSTTYPE      *dst_;                                                       \
    size_t        i_, count_;                                                 \
                                                                              \
    chunk_ = pst->dc_ind->data;                                               \
    dst_   = (DSTTYPE *)(void *)prim->indices->items;                         \
    while (chunk_) {                                                          \
      src_   = (const SRCTYPE *)(const void *)chunk_->data;                   \
      count_ = chunk_->usedsize / sizeof(SRCTYPE);                            \
      for (i_ = 0; i_ < count_; i_++)                                         \
        dst_[i_] = (DSTTYPE)src_[i_];                                         \
      dst_ += count_;                                                         \
      chunk_ = chunk_->next;                                                  \
    }                                                                         \
  } while (0)

#define PLY_COPY_INDICES_FROM_TEMP(DSTTYPE)                                   \
  do {                                                                        \
    switch (pst->indexComponentType) {                                        \
      case AKT_UBYTE:  PLY_COPY_INDICES(DSTTYPE, uint8_t);  break;            \
      case AKT_USHORT: PLY_COPY_INDICES(DSTTYPE, uint16_t); break;            \
      case AKT_UINT:   PLY_COPY_INDICES(DSTTYPE, uint32_t); break;            \
      default:                                                        break;  \
    }                                                                         \
  } while (0)

#define PLY_FMT_BINARY_LITTLE_0 AK_STR_PACK8_CHARS('b', 'i', 'n', 'a', 'r', 'y', '_', 'l')
#define PLY_FMT_BINARY_LITTLE_8 AK_STR_PACK8_CHARS('i', 't', 't', 'l', 'e', '_', 'e', 'n')
#define PLY_FMT_BINARY_LITTLE_16 AK_STR_PACK4_CHARS('d', 'i', 'a', 'n')
#define PLY_FMT_BINARY_BIG_0 AK_STR_PACK8_CHARS('b', 'i', 'n', 'a', 'r', 'y', '_', 'b')
#define PLY_FMT_BINARY_BIG_8 AK_STR_PACK8_CHARS('i', 'g', '_', 'e', 'n', 'd', 'i', 'a')

static
PLYProperty*
ply_prop_find(PLYElement      * __restrict elem,
              PLYPropertyType              semantic) {
  PLYProperty *prop;

  for (prop = elem->property; prop; prop = prop->next) {
    if (prop->semantic == semantic
        && !prop->islist
        && prop->typeDesc)
      return prop;
  }

  return NULL;
}

static
bool
ply_prop_enable(PLYProperty * __restrict prop, uint32_t slot) {
  if (!prop)
    return false;

  prop->ignore = false;
  prop->slot   = slot;
  prop->off    = (size_t)slot * sizeof(float);

  return true;
}

static
void
ply_accessor_source_type(AkAccessor  * __restrict acc,
                         PLYProperty * __restrict p0,
                         PLYProperty * __restrict p1,
                         PLYProperty * __restrict p2,
                         PLYProperty * __restrict p3) {
  AkTypeId type;

  if (!acc || !p0 || !p0->typeDesc)
    return;

  type = p0->typeDesc->typeId;
  if ((p1 && (!p1->typeDesc || p1->typeDesc->typeId != type))
      || (p2 && (!p2->typeDesc || p2->typeDesc->typeId != type))
      || (p3 && (!p3->typeDesc || p3->typeDesc->typeId != type)))
    return;

  acc->originalComponentType = type;
  acc->originallyNormalized  = false;
}

static
AkTypeDesc*
ply_type_desc_by_token(const char * __restrict tok, size_t len) {
  switch (len) {
    case 3:
      if (ak_str_pack4_fast(tok, 3) == AK_STR_PACK4_CHARS('i', 'n', 't', 0))
        return ak_typeDesc(AKT_INT);
      break;
    case 4: {
      uint32_t key;

      key = ak_str_pack4_fast(tok, 4);
      switch (key) {
        case AK_STR_PACK4_CHARS('u', 'i', 'n', 't'):
          return ak_typeDesc(AKT_UINT);
        case AK_STR_PACK4_CHARS('c', 'h', 'a', 'r'):
        case AK_STR_PACK4_CHARS('b', 'y', 't', 'e'):
        case AK_STR_PACK4_CHARS('i', 'n', 't', '8'):
          return ak_typeDesc(AKT_BYTE);
        default:
          break;
      }
      break;
    }
    case 5: {
      uint64_t key;

      key = ak_str_pack8_fast(tok, 5);
      switch (key) {
        case AK_STR_PACK8_CHARS('f', 'l', 'o', 'a', 't', 0, 0, 0):
          return ak_typeDesc(AKT_FLOAT);
        case AK_STR_PACK8_CHARS('u', 'c', 'h', 'a', 'r', 0, 0, 0):
        case AK_STR_PACK8_CHARS('u', 'b', 'y', 't', 'e', 0, 0, 0):
        case AK_STR_PACK8_CHARS('u', 'i', 'n', 't', '8', 0, 0, 0):
          return ak_typeDesc(AKT_UBYTE);
        case AK_STR_PACK8_CHARS('s', 'h', 'o', 'r', 't', 0, 0, 0):
        case AK_STR_PACK8_CHARS('i', 'n', 't', '1', '6', 0, 0, 0):
          return ak_typeDesc(AKT_SHORT);
        case AK_STR_PACK8_CHARS('i', 'n', 't', '3', '2', 0, 0, 0):
          return ak_typeDesc(AKT_INT);
        default:
          break;
      }
      break;
    }
    case 6: {
      uint64_t key;

      key = ak_str_pack8_fast(tok, 6);
      switch (key) {
        case AK_STR_PACK8_CHARS('d', 'o', 'u', 'b', 'l', 'e', 0, 0):
          return ak_typeDesc(AKT_DOUBLE);
        case AK_STR_PACK8_CHARS('u', 's', 'h', 'o', 'r', 't', 0, 0):
        case AK_STR_PACK8_CHARS('u', 'i', 'n', 't', '1', '6', 0, 0):
          return ak_typeDesc(AKT_USHORT);
        case AK_STR_PACK8_CHARS('u', 'i', 'n', 't', '3', '2', 0, 0):
          return ak_typeDesc(AKT_UINT);
        default:
          break;
      }
      break;
    }
    case 7: {
      uint64_t key;

      key = ak_str_pack8_fast(tok, 7);
      switch (key) {
        case AK_STR_PACK8_CHARS('f', 'l', 'o', 'a', 't', '3', '2', 0):
          return ak_typeDesc(AKT_FLOAT);
        case AK_STR_PACK8_CHARS('f', 'l', 'o', 'a', 't', '6', '4', 0):
          return ak_typeDesc(AKT_DOUBLE);
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  if (len < 32) {
    char name[32];

    memcpy(name, tok, len);
    name[len] = '\0';
    return ak_typeDescByName(name);
  }

  return NULL;
}

static
PLYPropertyType
ply_prop_semantic_by_token(const char * __restrict tok, size_t len) {
  switch (len) {
    case 1:
      switch (tok[0]) {
        case 'x': return PLY_PROP_X;
        case 'y': return PLY_PROP_Y;
        case 'z': return PLY_PROP_Z;
        case 's':
        case 'u': return PLY_PROP_S;
        case 't':
        case 'v': return PLY_PROP_T;
        case 'r': return PLY_PROP_R;
        case 'g': return PLY_PROP_G;
        case 'b': return PLY_PROP_B;
        case 'a': return PLY_PROP_A;
        default:  return PLY_PROP_UNSUPPORTED;
      }
    case 2:
      if (tok[0] == 'n') {
        switch (tok[1]) {
          case 'x': return PLY_PROP_NX;
          case 'y': return PLY_PROP_NY;
          case 'z': return PLY_PROP_NZ;
          default:  break;
        }
      }
      break;
    case 3:
      if (ak_str_pack4_fast(tok, 3) == AK_STR_PACK4_CHARS('r', 'e', 'd', 0))
        return PLY_PROP_R;
      break;
    case 4:
      if (ak_str_pack4_fast(tok, 4) == AK_STR_PACK4_CHARS('b', 'l', 'u', 'e'))
        return PLY_PROP_B;
      break;
    case 5: {
      uint64_t key;

      key = ak_str_pack8_fast(tok, 5);
      if (key == AK_STR_PACK8_CHARS('g', 'r', 'e', 'e', 'n', 0, 0, 0))
        return PLY_PROP_G;
      if (key == AK_STR_PACK8_CHARS('a', 'l', 'p', 'h', 'a', 0, 0, 0))
        return PLY_PROP_A;
      break;
    }
    default:
      break;
  }

  return PLY_PROP_UNSUPPORTED;
}

AK_INLINE
bool
ply_format_is_binary_little(const char * __restrict p) {
  return ak_str_pack8_fast(p, 8) == PLY_FMT_BINARY_LITTLE_0
      && ak_str_pack8_fast(p + 8, 8) == PLY_FMT_BINARY_LITTLE_8
      && ak_str_pack4_fast(p + 16, 4) == PLY_FMT_BINARY_LITTLE_16
      && (p[20] == ' ' || p[20] == '\t');
}

AK_INLINE
bool
ply_format_is_binary_big(const char * __restrict p) {
  return ak_str_pack8_fast(p, 8) == PLY_FMT_BINARY_BIG_0
      && ak_str_pack8_fast(p + 8, 8) == PLY_FMT_BINARY_BIG_8
      && p[16] == 'n'
      && (p[17] == ' ' || p[17] == '\t');
}

AK_HIDE
AkResult
ply_ply(AkDoc ** __restrict dest, const char * __restrict filepath) {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *plystr;
  char          *p, *b, *e;
  AkLibrary     *lib_vscene;
  AkVisualScene *scene;
  PLYElement    *elem;
  PLYProperty   *prop, *pit;
  PLYState       pstVal = {0}, *pst;
  size_t         plystrSize, off;
  bool           isAscii, isLittleEndian;
  char           c;

  if (ak_readfile(filepath, NULL, &plystr, &plystrSize) != AK_OK
      || !((p = plystr) && *p != '\0')) {
    if (plystr)
      ak_releasefile(plystr, plystrSize);
    return AK_ERR;
  }

  if (!(tolower(p[0]) == 'p' && tolower(p[1]) == 'l' && tolower(p[2]) == 'y')) {
    ak_releasefile(plystr, plystrSize);
    return AK_ERR;
  }
  
  p += 3;
  /* c  = *p; */

  NEXT_LINE

  elem = NULL;
  prop = NULL;
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  
  doc->inf                = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name          = filepath;
  doc->inf->dir           = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage     = true;
  doc->inf->ftype         = AK_FILE_TYPE_PLY;
  doc->inf->base.coordSys = AK_YUP;
  doc->coordSys           = AK_YUP; /* Default */
  
  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  /* libraries */
  doc->lib.geometries = ak_heap_calloc(heap, doc, sizeof(AkLibrary));
  lib_vscene          = ak_heap_calloc(heap, doc, sizeof(*lib_vscene));

  /* default scene */
  scene                  = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->node            = ak_heap_calloc(heap, doc, sizeof(*scene->node));
  scene->node->visible   = true;
  lib_vscene->chld       = &scene->base;
  lib_vscene->count      = 1;
  doc->lib.visualScenes  = lib_vscene;
  doc->scene.visualScene = ak_instanceMake(heap, doc, scene);

  /* parse state */
  memset(&pstVal, 0, sizeof(pstVal));
  pst              = &pstVal;
  pstVal.doc       = doc;
  pstVal.heap      = heap;
  pstVal.tmp = ak_heap_alloc(heap, doc, sizeof(void*));
  pstVal.node      = scene->node;
  pstVal.lib_geom  = doc->lib.geometries;

  isAscii        = false;
  isLittleEndian = false;
  pst->end       = (char *)plystr + plystrSize;
  
  /* parse header */
  do {
    /* skip spaces */
    SKIP_SPACES

    /* parse format but ignore version (for now maybe) */
    if (EQ6('f', 'o', 'r', 'm', 'a', 't')) {
      p += 7;

      SKIP_SPACES

      if (EQ5('a', 's', 'c', 'i', 'i')) {
        isAscii = true;
      } else if (ply_format_is_binary_little(p)) {
        isLittleEndian = true;
      } else if (ply_format_is_binary_big(p)) {
        isLittleEndian = false;
      } else {
        goto err; /* unknown format */
      }
    } else if (EQ7('e', 'l', 'e', 'm', 'e', 'n', 't')) {
      p += 8;

      elem = ak_heap_calloc(heap, pst->tmp, sizeof(*elem));
      
      if (!pst->element)
        pst->element = elem;

      if (pst->lastElement)
        pst->lastElement->next = elem;
      pst->lastElement = elem;

      if (EQ6('v', 'e', 'r', 't', 'e', 'x')) {
        AkUInt parsedCount;

        p += 7;
        SKIP_SPACES
        p = ak_strtoui_one_fast(p, &parsedCount);
        elem->count    = (uint32_t)parsedCount;
        elem->type     = PLY_ELEM_VERTEX;
        pst->vertcount = elem->count;
      } else if (EQ4('f', 'a', 'c', 'e')) {
        AkUInt parsedCount;

        p += 5;
        SKIP_SPACES
        p = ak_strtoui_one_fast(p, &parsedCount);
        elem->count = (uint32_t)parsedCount;
        elem->type  = PLY_ELEM_FACE;
      }
    } else if (elem && EQ8('p', 'r', 'o', 'p', 'e', 'r', 't', 'y')) {
      p += 9;
      SKIP_SPACES
      
      prop = ak_heap_calloc(heap, pst->tmp, sizeof(*prop));
      
      /* 1. type */
      
      b = p;
      while ((c = *++p) != '\0' && !AK_ARRAY_SPACE_CHECK);
      e = p;
      
      prop->islist = (size_t)(e - b) == 4
                  && ak_str_pack4_fast(b, 4) == AK_STR_PACK4_CHARS('l', 'i', 's', 't');
      
      if (!prop->islist) {
        prop->typeDesc = ply_type_desc_by_token(b, (size_t)(e - b));
      } else {
        /* 1.1 count type */
        SKIP_SPACES
        
        b = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_SEP_CHECK);
        e = p;
        
        prop->listCountTypeDesc = ply_type_desc_by_token(b, (size_t)(e - b));

        /* 1.2 type */
        SKIP_SPACES
        
        b = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_SEP_CHECK);
        e = p;
        
        prop->typeDesc = ply_type_desc_by_token(b, (size_t)(e - b));
      }
      
      /* 2. name */
      
      SKIP_SPACES
      
      b = p;
      while ((c = *++p) != '\0' && !AK_ARRAY_SEP_CHECK);
      e = p;

      prop->name = ak_heap_strndup(heap, doc, b, e - b);
      
      if (prop->typeDesc) {
        elem->buffsize += prop->typeDesc->size;
      } else if (!prop->islist && !isAscii) {
        /* we cannot traverse the binary because we don't know some types */
        goto err;
      }

      prop->semantic = ply_prop_semantic_by_token(b, (size_t)(e - b));
      if (prop->semantic == PLY_PROP_UNSUPPORTED)
        prop->ignore = true;
      
      if (!elem->property) {
        elem->property = prop;
      } else {
        pit = elem->property;
        while (pit->next)
          pit = pit->next;

        pit->next  = prop;
        prop->prev = pit;
      }
    } else if (EQT7('e', 'n', 'd', '_', 'h', 'e', 'a')) {
      NEXT_LINE
      break;
    }

    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  /* prepare property offsets/slots */
  off  = 0;
  elem = pst->element;

  while (elem) {
    pit = elem->property;
    if (elem->type == PLY_ELEM_VERTEX) {
      PLYProperty *px, *py, *pz;
      PLYProperty *pnx, *pny, *pnz;
      PLYProperty *ps, *pt;
      PLYProperty *pr, *pg, *pb, *pa;
      size_t byteSffset;
      uint32_t slot;
      
      byteSffset = 0;
      slot        = 0;
      elem->buff = ak_heap_calloc(heap, pst->doc, sizeof(*elem->buff));

      while (pit) {
        pit->ignore = true;
        pit = pit->next;
      }

      px = ply_prop_find(elem, PLY_PROP_X);
      py = ply_prop_find(elem, PLY_PROP_Y);
      pz = ply_prop_find(elem, PLY_PROP_Z);
      if (!ply_prop_enable(px, slot++)
          || !ply_prop_enable(py, slot++)
          || !ply_prop_enable(pz, slot++))
        goto err;

      pst->ac_pos = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC3,
                           AKT_FLOAT, elem->count, elem->buff);
      ply_accessor_source_type(pst->ac_pos, px, py, pz, NULL);

      pnx = ply_prop_find(elem, PLY_PROP_NX);
      pny = ply_prop_find(elem, PLY_PROP_NY);
      pnz = ply_prop_find(elem, PLY_PROP_NZ);
      if (pnx && pny && pnz) {
        ply_prop_enable(pnx, slot++);
        ply_prop_enable(pny, slot++);
        ply_prop_enable(pnz, slot++);
        pst->ac_nor = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC3,
                             AKT_FLOAT, elem->count, elem->buff);
        ply_accessor_source_type(pst->ac_nor, pnx, pny, pnz, NULL);
      }

      ps = ply_prop_find(elem, PLY_PROP_S);
      pt = ply_prop_find(elem, PLY_PROP_T);
      if (ps && pt) {
        ply_prop_enable(ps, slot++);
        ply_prop_enable(pt, slot++);
        pst->ac_tex = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC2,
                             AKT_FLOAT, elem->count, elem->buff);
        ply_accessor_source_type(pst->ac_tex, ps, pt, NULL, NULL);
      }

      pr = ply_prop_find(elem, PLY_PROP_R);
      pg = ply_prop_find(elem, PLY_PROP_G);
      pb = ply_prop_find(elem, PLY_PROP_B);
      pa = ply_prop_find(elem, PLY_PROP_A);
      if (pr && pg && pb) {
        ply_prop_enable(pr, slot++);
        ply_prop_enable(pg, slot++);
        ply_prop_enable(pb, slot++);
        if (pa) {
          ply_prop_enable(pa, slot++);
          pst->ac_rgb = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC4,
                               AKT_FLOAT, elem->count, elem->buff);
          ply_accessor_source_type(pst->ac_rgb, pr, pg, pb, pa);
        } else {
          pst->ac_rgb = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC3,
                             AKT_FLOAT, elem->count, elem->buff);
          ply_accessor_source_type(pst->ac_rgb, pr, pg, pb, NULL);
        }
      }

      elem->knownCount = slot;
      pst->byteStride  = slot * (uint32_t)sizeof(float);
      off              = pst->byteStride;

      /* empty buffer */
      if (off < 1)
        goto err;
      
      /* alloc buffer for vertex element */
      pst->vertBuffsize  = off * elem->count;
      elem->buff->length = pst->vertBuffsize;
      elem->buff->data   = ak_heap_alloc(heap, elem->buff, elem->buff->length);
      flist_sp_insert(&pst->doc->lib.buffers, elem->buff);

      /* prepare accessors' misssing params */
      if (pst->ac_pos) {
        pst->ac_pos->byteLength = pst->vertBuffsize;
        pst->ac_pos->byteStride = pst->byteStride;
        byteSffset += sizeof(float) * 3;
      }
      
      if (pst->ac_nor) {
        pst->ac_nor->byteLength = pst->vertBuffsize;
        pst->ac_nor->byteStride = pst->byteStride;
        pst->ac_nor->byteOffset = byteSffset;
        byteSffset += sizeof(float) * 3;
      }

      if (pst->ac_tex) {
        pst->ac_tex->byteStride = pst->byteStride;
        pst->ac_tex->byteLength = pst->vertBuffsize;
        pst->ac_tex->byteOffset = byteSffset;
        byteSffset += sizeof(float) * 2;
      }
       
      if (pst->ac_rgb) {
        pst->ac_rgb->byteStride = pst->byteStride;
        pst->ac_rgb->byteLength = pst->vertBuffsize;
        pst->ac_rgb->byteOffset = byteSffset;
        /* byteSffset += sizeof(float) * pst->ac_rgb->componentCount; */
      }
    }

    elem = elem->next;
  }

  /* parse */
  if (isAscii) {
    ply_ascii(p, pst);
  } else {
    ply_bin(p, pst, isLittleEndian);
  }

  io_postscript(doc);

  *dest = doc;

  /* cleanup */
  ak_free(pst->tmp);
  ak_releasefile(plystr, plystrSize);

  return AK_OK;
  
err:
  ak_free(pst->tmp);
  ak_free(doc);
  ak_releasefile(plystr, plystrSize);
  return AK_ERR;
}

AK_HIDE
void
ply_finish(PLYState * __restrict pst) {
  AkHeap             *heap;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkInstanceGeometry *instGeom;
  AkTriangles        *tri;

  /* Buffer > Accessor > Input > Prim > Mesh > Geom > InstanceGeom > Node */
  
  heap = pst->heap;
  mesh = ak_allocMesh(pst->heap, pst->lib_geom, &geom);

  if (pst->dc_ind && pst->dc_ind->itemcount > 0) {
    tri            = ak_heap_calloc(pst->heap, ak_objFrom(mesh), sizeof(*tri));
    tri->mode      = AK_TRIANGLES;
    tri->base.type = AK_PRIMITIVE_TRIANGLES;
    prim           = (AkMeshPrimitive *)tri;
  } else {
    prim       = ak_heap_calloc(pst->heap, ak_objFrom(mesh), sizeof(*prim));
    prim->type = AK_PRIMITIVE_POINTS;
  }

  prim->indexStride    = 1;
  prim->nPolygons      = pst->dc_ind && pst->dc_ind->itemcount > 0
                         ? pst->count / 3
                         : pst->vertcount;
  prim->mesh           = mesh;
  mesh->primitive      = prim;
  mesh->primitiveCount = 1;

  /* add to library */
  geom->base.next      = pst->lib_geom->chld;
  pst->lib_geom->chld  = &geom->base;
  pst->lib_geom->count = 1;
  
  /* make instance geeometry and attach to the root node  */
  instGeom = ak_instanceMakeGeom(heap, pst->node, geom);
  if (pst->node->geometry) {
    pst->node->geometry->base.prev = (void *)instGeom;
    instGeom->base.next            = (void *)pst->node->geometry;
  }

  pst->node->geometry = instGeom;
  
  /* positions */
  if (pst->ac_pos)
    prim->pos = io_input(heap, prim, pst->ac_pos,
                         AK_INPUT_POSITION, _s_POSITION, 0);

  /* normals */
  if (pst->ac_nor)
    io_input(heap, prim, pst->ac_nor, AK_INPUT_NORMAL, _s_NORMAL, 0);

  /* tex coords */
  if (pst->ac_tex)
    io_input(heap, prim, pst->ac_tex, AK_INPUT_TEXCOORD, _s_TEXCOORD, 0);
  
  /* vertex colors */
  if (pst->ac_rgb)
    io_input(heap, prim, pst->ac_rgb, AK_INPUT_COLOR, _s_COLOR, 0);
  
  /* indices */
  {
    AkTypeId componentType;
    AkUInt   maxIndex;

    if (!pst->dc_ind || pst->dc_ind->itemcount == 0)
      return;

    maxIndex      = pst->indexMax;
    componentType = ak_indexComponentTypeForMax(maxIndex);

    prim->indices = ak_indexArrayAlloc(heap,
                                       tri,
                                       pst->dc_ind->itemcount,
                                       componentType);
    if (!prim->indices)
      return;

    prim->indices->count = pst->dc_ind->itemcount;
    prim->indices->max   = maxIndex;

    if (pst->dc_ind->itemsize == ak_indexComponentSize(componentType)) {
      ak_data_join(pst->dc_ind, prim->indices->items, 0, 0);
    } else {
      switch (componentType) {
        case AKT_UBYTE:
          PLY_COPY_INDICES_FROM_TEMP(uint8_t);
          break;
        case AKT_USHORT:
          PLY_COPY_INDICES_FROM_TEMP(uint16_t);
          break;
        case AKT_UINT:
          PLY_COPY_INDICES_FROM_TEMP(uint32_t);
          break;
        default:
          break;
      }
    }
  }
}
