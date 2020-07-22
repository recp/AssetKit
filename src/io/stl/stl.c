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

AkResult AK_HIDE
stl_stl(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *stlstr;
  char          *p;
  AkLibrary     *lib_geom, *lib_vscene;
  AkVisualScene *scene;
  STLState       sstVal = {0}, *sst;
  size_t         stlstrSize;
  AkResult       ret;
  bool           isAscii;

  if ((ret = ak_readfile(filepath, "rb", &stlstr, &stlstrSize)) != AK_OK
      || !((p = stlstr) && *p != '\0'))
    return AK_ERR;

  if (p[0] == 's'
   && p[1] == 'o'
   && p[2] == 'l'
   && p[3] == 'i'
   && p[4] == 'd') {
    isAscii = true;
  } else if (p[0] != '\0' && stlstrSize > 80) {
    isAscii = false;
  } else {
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
  memset(&sstVal, 0, sizeof(sstVal));
  sst              = &sstVal;
  sstVal.doc       = doc;
  sstVal.heap      = heap;
  sstVal.tmpParent = ak_heap_alloc(heap, doc, sizeof(void*));
  sstVal.node      = scene->node;
  sstVal.lib_geom  = doc->lib.geometries;
  
  sst->dc_ind    = ak_data_new(sst->tmpParent, 128, sizeof(int32_t), NULL);
  sst->dc_pos    = ak_data_new(sst->tmpParent, 128, sizeof(vec3),    NULL);
  sst->dc_nor    = ak_data_new(sst->tmpParent, 128, sizeof(vec3),    NULL);
  sst->dc_vcount = ak_data_new(sst->tmpParent, 128, sizeof(int32_t), NULL);

  if (!isAscii) {
    stl_binary(sst, p);
  } else {
    stl_ascii(sst, p);
  }
  
  sst_finish(sst);
  io_postscript(doc);
  
  *dest = doc;

  /* cleanup */
  ak_free(sst->tmpParent);

  return AK_OK;
}

AK_HIDE
void
stl_binary(STLState * __restrict sst, char * __restrict p) {
  vec4     v, n;
  uint32_t count,  nTriangles, i;
  char     c;

  c = '\0';
  
  /* skip 80-char header */
  p += 80;

  /* parse integers from little endian to native */
  le_32(nTriangles, p);

  count      = nTriangles * 3;
  sst->maxVC = 3;

  for (i = 0; i < nTriangles; i++) {
    /* normal */
    le_32(v[0], p);
    le_32(v[1], p);
    le_32(v[2], p);
    
    ak_data_append(sst->dc_nor, n);
    ak_data_append(sst->dc_nor, n);
    ak_data_append(sst->dc_nor, n);
    
    /* vertex */
    le_32(v[0], p);
    le_32(v[1], p);
    le_32(v[2], p);
    ak_data_append(sst->dc_pos, v);
    
    le_32(v[0], p);
    le_32(v[1], p);
    le_32(v[2], p);
    ak_data_append(sst->dc_pos, v);
    
    le_32(v[0], p);
    le_32(v[1], p);
    le_32(v[2], p);
    ak_data_append(sst->dc_pos, v);
    p += 2;
  }
  
  sst->count = count;
}

AK_HIDE
void
stl_ascii(STLState * __restrict sst, char * __restrict p) {
  vec4     v, n;
  uint32_t vc, count;
  char     c;

  c     = '\0';
  count = 0;

  NEXT_LINE

  /* parse ASCII STL */
  do {
    /* skip spaces */
    SKIP_SPACES

    if (EQ5('f', 'a', 'c', 'e', 't')) {
      p += 6;
      
      SKIP_SPACES
      
      if (EQ6('n', 'o', 'r', 'm', 'a', 'l')) {
        p += 7;
        memset(v, 0, sizeof(vec4));
        ak_strtof_line(p, 0, 3, n);
        /* ak_data_append(sst->dc_nor, n); */
        
        NEXT_LINE
        SKIP_SPACES
        
        /* parse each vertex */
        if (EQ5('o', 'u', 't', 'e', 'r')) {
          NEXT_LINE
          
          vc = 0;
          
          /* parse vertices */
          while (c != '\0') {
            SKIP_SPACES
            if (EQ6('v', 'e', 'r', 't', 'e', 'x')) {
              p += 7;
              memset(v, 0, sizeof(vec4));
              ak_strtof_line(p, 0, 3, v);
              ak_data_append(sst->dc_nor, n); /* duplicate normal */
              ak_data_append(sst->dc_pos, v);

              vc++;
            } else if (EQT7('e', 'n', 'd', 'l', 'o', 'o', 'p')) {
              break;
            }

            NEXT_LINE
          } /* vertex */

          count += vc;
          sst->maxVC = GLM_MAX(sst->maxVC, vc);
          ak_data_append(sst->dc_vcount, &vc);
        } /* outer loop */
      } /* normal */
    } /* facet */

    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);
  
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
  
  prim->count          = sst->count;
  prim->mesh           = mesh;
  mesh->primitive      = prim;
  mesh->primitiveCount = 1;

  /* add to library */
  geom->base.next     = sst->lib_geom->chld;
  sst->lib_geom->chld = &geom->base;
  
  /* make instance geeometry and attach to the root node  */
  instGeom = ak_instanceMakeGeom(heap, sst->node, geom);
  if (sst->node->geometry) {
    sst->node->geometry->base.prev = (void *)instGeom;
    instGeom->base.next            = &sst->node->geometry->base;
  }

  instGeom->base.next = (void *)sst->node->geometry;
  sst->node->geometry = instGeom;
  
  prim->pos = io_addInput(heap, sst->dc_pos, prim, AK_INPUT_POSITION,
                          "POSITION", AK_COMPONENT_SIZE_VEC3, AKT_FLOAT, 0);

  if (sst->dc_nor->itemcount > 0) {
    io_addInput(heap, sst->dc_nor, prim, AK_INPUT_NORMAL,
                "NORMAL", AK_COMPONENT_SIZE_VEC3, AKT_FLOAT, 1);
  }

  /* cleanup */
  if (sst->dc_ind) {
    ak_free(sst->dc_ind);
    ak_free(sst->dc_pos);
    ak_free(sst->dc_nor);
    ak_free(sst->dc_vcount);
  }
  
  sst->dc_ind    = NULL;
  sst->dc_pos    = NULL;
  sst->dc_nor    = NULL;
  sst->dc_vcount = NULL;
}
