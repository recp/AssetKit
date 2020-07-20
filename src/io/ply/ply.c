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
//#include "postscript.h"
#include "strpool.h"
#include "../../id.h"
#include "../../data.h"
#include "../../../include/ak/path.h"
#include "../common/util.h"

AkResult AK_HIDE
ply_ply(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *stlstr;
  char          *p, *b, *e;
  AkTypeDesc    *typeDesc;
  AkLibrary     *lib_geom, *lib_vscene;
  AkVisualScene *scene;
  PLYElement    *elem;
  PLYProperty   *prop;
  PLYState       pstVal = {0}, *pst;
  size_t         stlstrSize;
  AkResult       ret;
  bool           isAscii, isBigEndian;
  char           c;

  if ((ret = ak_readfile(filepath, "rb", &stlstr, &stlstrSize)) != AK_OK
      || !((p = stlstr) && *p != '\0'))
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
  lib_vscene = ak_heap_calloc(heap, doc, sizeof(*lib_vscene));
  
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
  pstVal.tmpParent = ak_heap_alloc(heap, doc, sizeof(void*));
  pstVal.node      = scene->node;
  pstVal.lib_geom  = doc->lib.geometries;
  
  pst->dc_ind    = ak_data_new(pst->tmpParent, 128, sizeof(int32_t), NULL);
  pst->dc_pos    = ak_data_new(pst->tmpParent, 128, sizeof(vec3),    NULL);
  pst->dc_nor    = ak_data_new(pst->tmpParent, 128, sizeof(vec3),    NULL);
  pst->dc_vcount = ak_data_new(pst->tmpParent, 128, sizeof(int32_t), NULL);

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
      } else if (strncmp(p, _s_ply_binary_little_endian, 21) == 0) {
        isBigEndian = false;
      } else if (strncmp(p, _s_ply_binary_big_endian, 18) == 0) {
        isBigEndian = true;
      } else {
        goto err; /* unknown format */
      }
    } else if (EQ7('e', 'l', 'e', 'm', 'e', 'n', 't')) {
      p += 8;
      
      elem = ak_heap_calloc(heap, pst->tmpParent, sizeof(*elem));
      
      if (!pst->element)
        pst->element = elem;

      if (pst->lastElement)
        pst->lastElement->next = elem;
      pst->lastElement = elem;

      if (EQ6('v', 'e', 'r', 't', 'e', 'x')) {
        p += 7;
        SKIP_SPACES
        elem->count = strtoul(p, &p, 10);
        elem->type  = PLY_ELEM_VERTEX;
      } else if (EQ4('f', 'a', 'c', 'e')) {
        p += 5;
        SKIP_SPACES
        elem->count = strtoul(p, &p, 10);
        elem->type  = PLY_ELEM_FACE;
      }
    } else if (EQ8('p', 'r', 'o', 'p', 'e', 'r', 't', 'y')) {
      p += 9;
      SKIP_SPACES
      
      prop = ak_heap_calloc(heap, pst->tmpParent, sizeof(*prop));
      
      if (!elem->property)
        elem->property = prop;

      if (elem->lastProperty)
        elem->lastProperty->next = prop;
      elem->lastProperty = prop;
      
      /* 1. type */
      
      b = p;
      while ((c = *++p) != '\0' && !AK_ARRAY_SPACE_CHECK);
      e = p;
      
      prop->islist = b[0] == 'l'
                  && b[1] == 'i'
                  && b[2] == 's'
                  && b[3] == 't';
      
      if (!prop->islist)
        prop->typestr = ak_heap_strndup(heap, doc, b, e - b);
      else {
        /* 1.1 count type */
        SKIP_SPACES
        
        b = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_SEP_CHECK);
        e = p;
        
        prop->listCountType = ak_heap_strndup(heap, doc, b, e - b);
        
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
      
      if (e - b == 1) {
        switch (b[0]) {
          case 'x': prop->type = PLY_PROP_X; break;
          case 'y': prop->type = PLY_PROP_Y; break;
          case 'z': prop->type = PLY_PROP_Z; break;
          case 's':
          case 'u': prop->type = PLY_PROP_S; break;
          case 't':
          case 'v': prop->type = PLY_PROP_T; break;
          case 'r': prop->type = PLY_PROP_R; break;
          case 'g': prop->type = PLY_PROP_G; break;
          case 'b': prop->type = PLY_PROP_B; break;
          default:
            prop->type = PLY_PROP_UNSUPPORTED;
            prop->ignore   = true;
            break;
        }
      } else if (e - b == 2) {
        switch (b[0]) {
          case 'n':
            switch (b[1]) {
              case 'x': prop->type = PLY_PROP_NX; break;
              case 'y': prop->type = PLY_PROP_NY; break;
              case 'z': prop->type = PLY_PROP_NZ; break;
              default:
                prop->type = PLY_PROP_UNSUPPORTED;
                prop->ignore   = true;
                break;
            }
            break;
          default:
            prop->type = PLY_PROP_UNSUPPORTED;
            prop->ignore   = true;
            break;
        }
      }
    } else if (EQT7('e', 'n', 'd', '_', 'h', 'e', 'a')) {
      NEXT_LINE
      break;
    }

    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  *dest = doc;

  /* cleanup */
  ak_free(pst->tmpParent);

  return AK_OK;
  
err:
  ak_free(pst->tmpParent);
  ak_free(doc);
  return AK_ERR;
}
