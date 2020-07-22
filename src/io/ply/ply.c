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
*/

#include "ply.h"
#include "common.h"
#include "postscript.h"
#include "../../id.h"
#include "../../data.h"
#include "../../../include/ak/path.h"
#include "../common/util.h"

AkResult AK_HIDE
ply_ply(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *plystr;
  char          *p, *b, *e;
  AkLibrary     *lib_geom, *lib_vscene;
  AkVisualScene *scene;
  PLYElement    *elem;
  PLYProperty   *prop, *pit;
  PLYState       pstVal = {0}, *pst;
  size_t         plystrSize, off;
  uint32_t       i;
  AkResult       ret;
  bool           isAscii, isBigEndian;
  char           c;

  if ((ret = ak_readfile(filepath, "rb", &plystr, &plystrSize)) != AK_OK
      || !((p = plystr) && *p != '\0'))
    return AK_ERR;

  if (!(p[0] == 'p' && p[1] == 'l' && p[2] == 'y'))
    return AK_ERR;
  
  p += 3;
  c  = *p;

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
  doc->lib.geometries = ak_heap_calloc(heap, doc, sizeof(*lib_geom));
  lib_vscene          = ak_heap_calloc(heap, doc, sizeof(*lib_vscene));

  /* default scene */
  scene                  = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->node            = ak_heap_calloc(heap, doc, sizeof(*scene->node));
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

  isAscii     = false;
  isBigEndian = false;

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
      } else if (strncmp(p, "binary_little_endian", 21) == 0) {
        isBigEndian = false;
      } else if (strncmp(p, "binary_big_endian", 18) == 0) {
        isBigEndian = true;
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
        p += 7;
        SKIP_SPACES
        elem->count = (uint32_t)strtoul(p, &p, 10);
        elem->type  = PLY_ELEM_VERTEX;
      } else if (EQ4('f', 'a', 'c', 'e')) {
        p += 5;
        SKIP_SPACES
        elem->count = (uint32_t)strtoul(p, &p, 10);
        elem->type  = PLY_ELEM_FACE;
      }
    } else if (EQ8('p', 'r', 'o', 'p', 'e', 'r', 't', 'y')) {
      p += 9;
      SKIP_SPACES
      
      prop = ak_heap_calloc(heap, pst->tmp, sizeof(*prop));
      
      /* 1. type */
      
      b = p;
      while ((c = *++p) != '\0' && !AK_ARRAY_SPACE_CHECK);
      e = p;
      
      prop->islist = b[0] == 'l'
                  && b[1] == 'i'
                  && b[2] == 's'
                  && b[3] == 't';
      
      if (!prop->islist) {
        prop->typestr  = ak_heap_strndup(heap, doc, b, e - b);
        prop->typeDesc = ak_typeDescByName(prop->typestr);
      } else {
        /* 1.1 count type */
        SKIP_SPACES
        
        b = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_SEP_CHECK);
        e = p;
        
        prop->listCountType     = ak_heap_strndup(heap, doc, b, e - b);
        prop->listCountTypeDesc = ak_typeDescByName(prop->listCountType);

        /* 1.2 type */
        SKIP_SPACES
        
        b = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_SEP_CHECK);
        e = p;
        
        prop->typestr = ak_heap_strndup(heap, doc, b, e - b);
      }
      
      /* 2. name */
      
      SKIP_SPACES
      
      b = p;
      while ((c = *++p) != '\0' && !AK_ARRAY_SEP_CHECK);
      e = p;

      prop->name = ak_heap_strndup(heap, doc, b, e - b);
      
      if (prop->typeDesc)
        elem->buffsize += prop->typeDesc->size;

      if (e - b == 1) {
        switch (b[0]) {
          case 'x': prop->semantic = PLY_PROP_X; break;
          case 'y': prop->semantic = PLY_PROP_Y; break;
          case 'z': prop->semantic = PLY_PROP_Z; break;
          case 's':
          case 'u': prop->semantic = PLY_PROP_S; break;
          case 't':
          case 'v': prop->semantic = PLY_PROP_T; break;
          case 'r': prop->semantic = PLY_PROP_R; break;
          case 'g': prop->semantic = PLY_PROP_G; break;
          case 'b': prop->semantic = PLY_PROP_B; break;
          default:
            prop->semantic   = PLY_PROP_UNSUPPORTED;
            prop->ignore = true;
            break;
        }
      } else if (e - b == 2) {
        switch (b[0]) {
          case 'n':
            switch (b[1]) {
              case 'x': prop->semantic = PLY_PROP_NX; break;
              case 'y': prop->semantic = PLY_PROP_NY; break;
              case 'z': prop->semantic = PLY_PROP_NZ; break;
              default:
                prop->semantic   = PLY_PROP_UNSUPPORTED;
                prop->ignore = true;
                break;
            }
            break;
          default:
            prop->semantic   = PLY_PROP_UNSUPPORTED;
            prop->ignore = true;
            break;
        }
      }
      
      if (!elem->property) {
        elem->property = prop;
      } else {
        PLYProperty *last_prop;
        
        /* insert propety by ORDER */
        last_prop = pit = elem->property;
        while (pit) {
          if ((int)prop->semantic < (int)pit->semantic) {
            if (pit->prev) {
              pit->prev->next = prop;
              prop->prev      = pit->prev;
            }

            prop->next = pit;
            pit->prev  = prop;
            
            if (pit == elem->property)
              elem->property = prop;
            
            break;
          }

          last_prop = pit;
          pit       = pit->next;
        }
        
        /* couldn't add, so add to last */
        if (!pit && last_prop) {
          last_prop->next = prop;
          prop->prev      = last_prop;
        }
      }
    } else if (EQT7('e', 'n', 'd', '_', 'h', 'e', 'a')) {
      NEXT_LINE
      break;
    }

    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  /* prepare property offsets/slots */
  i    = off = 0;
  elem = pst->element;
  while (elem) {
    pit = elem->property;
    if (elem->type == PLY_ELEM_VERTEX) {
      size_t byteSffset;
      
      byteSffset = 0;
      elem->buff = ak_heap_calloc(heap, pst->doc, sizeof(*elem->buff));

      while (pit) {
        if (!pit->ignore) {
          /* validate, check missing properties in the group */
          if (pit->semantic == PLY_PROP_X) {
            if ((!pit->next || pit->next->semantic != PLY_PROP_Y)
                ||(!pit->next->next || pit->next->next->semantic != PLY_PROP_Z))
              goto err; /* we cannot load this PLY, TODO: */
            
            /* alloc input and accessor for positions */
            pst->ac_pos = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC3,
                                 AKT_FLOAT, elem->count, elem->buff);
          }
          
          if (pit->semantic == PLY_PROP_NX) {
            if ((!pit->next || pit->next->semantic != PLY_PROP_NY)
                ||(!pit->next->next || pit->next->next->semantic != PLY_PROP_NZ))
              goto ign; /* we cannot load this PLY, TODO: */
            
            /* alloc input and accessor for normals */
            pst->ac_nor = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC3,
                                 AKT_FLOAT, elem->count, elem->buff);
          }
          
          if (pit->semantic == PLY_PROP_S) {
            if ((!pit->next || pit->next->semantic != PLY_PROP_T))
              goto ign; /* ignore, TODO: */
            
            /* alloc input and accessor for tex coords */
            pst->ac_tex = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC2,
                                 AKT_FLOAT, elem->count, elem->buff);
          }
          
          if (pit->semantic == PLY_PROP_R) {
            if ((!pit->next || pit->next->semantic != PLY_PROP_G)
                ||(!pit->next->next || pit->next->next->semantic != PLY_PROP_B))
              goto ign; /* ignore, TODO: */
            
            /* alloc input and accessor for vertex colors */
            pst->ac_rgb = io_acc(heap, doc, AK_COMPONENT_SIZE_VEC3,
                                 AKT_FLOAT, elem->count, elem->buff);
          }
          
          pit->slot = i++;
          pit->off  = off;
          /* TODO: currently all are floats */
          off      += sizeof(float); /* pit->typeDesc->size; */
          
          if (pit->typeDesc)
            pst->byteStride += pit->typeDesc->size;
          elem->knownCount++;
        }
      ign:
        pit = pit->next;
      }
      
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
        byteSffset += sizeof(float) * 3;
      }
    }

    elem = elem->next;
  }
  
  /* parse */
  if (isAscii) {
    ply_ascii(p, pst);
  }

  ply_postscript(pst);

  *dest = doc;

  /* cleanup */
  ak_free(pst->tmp);

  return AK_OK;
  
err:
  ak_free(pst->tmp);
  ak_free(doc);
  return AK_ERR;
}

AK_HIDE
void
ply_ascii(char * __restrict src, PLYState * __restrict pst) {
  char        *p;
  float       *b;
  PLYElement  *elem;
  PLYProperty *prop;
  AkBuffer    *buff;
  char         c;
  uint32_t     i, stride;
  
  p    = src;
  elem = pst->element;

  while (elem) {
    if (elem->type == PLY_ELEM_VERTEX) {
      buff   = elem->buff;
      b      = buff->data; /* TODO: all vertices are floats for now */
      stride = elem->knownCount;
      i      = 0;
      c      = *p;

      do {
        SKIP_SPACES

        prop = elem->property;
        while (prop) {
          if (!prop->ignore)
            b[prop->slot] = strtof(p, &p);
          prop = prop->next;
        }

        b += stride;

        NEXT_LINE

        if (++i >= elem->count)
          break;
      } while (p && p[0] != '\0');
    } else if (elem->type == PLY_ELEM_FACE) {
      AkUInt *f, center, fc, j, count, last_fc;
      
      pst->dc_ind = ak_data_new(pst->tmp, 128, sizeof(AkUInt), NULL);
      c           = *p;
      f           = NULL;
      i           = 0;
      count       = 0;
      last_fc     = 0;

      do {
        SKIP_SPACES
        
        fc = (AkUInt)strtol(p, &p, 10);
        if (fc >= 3) {
          if (!f && last_fc != fc)
            f = alloca(sizeof(AkUInt) * fc);
          
          for (j = 0; j < fc; j++)
            f[j] = (AkUInt)strtol(p, &p, 10);
          
          center = f[0];
          for (j = 0; j < fc - 2; j++) {
            ak_data_append(pst->dc_ind, &center);
            ak_data_append(pst->dc_ind, &f[j + 1]);
            ak_data_append(pst->dc_ind, &f[j + 2]);
            count += 3;
          }
        }

        last_fc = fc;

        NEXT_LINE

        if (++i >= elem->count)
          break;
      } while (p && p[0] != '\0');
      
      pst->count = count;
    }
    elem = elem->next;
  }
  
  ply_finish(pst);
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

  tri            = ak_heap_calloc(pst->heap, ak_objFrom(mesh), sizeof(*tri));
  tri->mode      = AK_TRIANGLES;
  tri->base.type = AK_PRIMITIVE_TRIANGLES;
  prim           = (AkMeshPrimitive *)tri;

  prim->indexStride    = 1;
  prim->count          = pst->count;
  prim->mesh           = mesh;
  mesh->primitive      = prim;
  mesh->primitiveCount = 1;

  /* add to library */
  geom->base.next     = pst->lib_geom->chld;
  pst->lib_geom->chld = &geom->base;
  
  /* make instance geeometry and attach to the root node  */
  instGeom = ak_instanceMakeGeom(heap, pst->node, geom);
  if (pst->node->geometry) {
    pst->node->geometry->base.prev = (void *)instGeom;
    instGeom->base.next            = &pst->node->geometry->base;
  }

  instGeom->base.next = (void *)pst->node->geometry;
  pst->node->geometry = instGeom;
  
  /* positions */
  if (pst->ac_pos)
    prim->pos = io_input(heap, prim, pst->ac_pos,
                         AK_INPUT_SEMANTIC_POSITION, "POSITION", 0);

  /* normals */
  if (pst->ac_nor)
    io_input(heap, prim, pst->ac_nor, AK_INPUT_SEMANTIC_NORMAL, "NORMAL", 0);

  /* tex coords */
  if (pst->ac_tex)
    io_input(heap, prim, pst->ac_nor, AK_INPUT_SEMANTIC_TEXCOORD, "TEXCOORD", 0);
  
  /* vertex colors */
  if (pst->ac_rgb)
    io_input(heap, prim, pst->ac_nor, AK_INPUT_SEMANTIC_COLOR, "COLOR", 0);
  
  /* indices */
  prim->indices = ak_heap_calloc(heap,
                                 tri,
                                 sizeof(*prim->indices)
                                 + pst->dc_ind->usedsize);
  prim->indices->count = pst->dc_ind->itemcount;
  ak_data_join(pst->dc_ind, prim->indices->items, 0, 0);
}
