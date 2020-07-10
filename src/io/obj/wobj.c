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
   https://all3dp.com/1/obj-file-format-3d-printing-cad/
*/

#include "wobj.h"
#include "common.h"
#include "postscript.h"
#include "../../id.h"
#include "../../data.h"
#include "../../../include/ak/path.h"

#define SKIP_SPACES                                                           \
  {                                                                           \
    while (c != '\0' && AK_ARRAY_SPACE_CHECK) c = *++p;                       \
    if (c == '\0')                                                            \
      continue; /* to break loop */                                           \
  }

#define NEXT_LINE                                                             \
  do {                                                                        \
    while (p                                                                  \
           && p[0] != '\0'                                                    \
           && (c = *++p) != '\0'                                              \
           && !AK_ARRAY_NLINE_CHECK);                                         \
                                                                              \
    while (p                                                                  \
           && p[0] != '\0'                                                    \
           && (c = *++p) != '\0'                                              \
           && AK_ARRAY_NLINE_CHECK);                                          \
  } while(0);

typedef struct AkObjFace {
  long vp;
  long vt;
  long vn;
} AkObjFace;

static
void
ak_wobjFreeDupl(RBTree *tree, RBNode *node);

AkMesh*
ak_allocMesh(AkHeap      * __restrict heap,
             AkLibrary   * __restrict memp,
             AkGeometry ** __restrict geomLink) {
  AkGeometry *geom;
  AkObject   *meshObj;
  AkMesh     *mesh;

  /* create geometries */
  geom              = ak_heap_calloc(heap, memp, sizeof(*geom));
  geom->materialMap = ak_map_new(ak_cmp_str);

  /* destroy heap with this object */
  ak_setAttachedHeap(geom, geom->materialMap->heap);

  meshObj     = ak_objAlloc(heap, geom, sizeof(AkMesh), AK_GEOMETRY_MESH, true);
  geom->gdata = meshObj;
  mesh        = ak_objGet(meshObj);
  mesh->geom  = geom;
  
  if (geomLink)
    *geomLink = geom;

  return mesh;
}

AkBuffer*
ak_buffFromDataCtx(AkHeap        * __restrict heap,
                   void          * __restrict memp,
                   AkDataContext * __restrict dctx) {
  AkBuffer *buff;

  buff = ak_heap_calloc(heap, memp, sizeof(*buff));

  return buff;
}

void
wobj_finishObject(WOState * __restrict wst) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkInstanceGeometry *instGeom;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkPolygon          *poly;
  AkBuffer           *buff_p;
  AkAccessor         *acc_p;
  AkInput            *inp_p;

  /* TODO: Release resources */

  if (!wst->obj.geom)
    return;

  /* Buffer > Accessor > Input > Prim > Mesh > Geom > InstanceGeom > Node */

  heap = wst->heap;
  doc  = wst->doc;
  geom = wst->obj.geom;
  mesh = ak_objGet(geom->gdata);
  poly = (void *)mesh->primitive;

  /* add to library */
  geom->base.next     = wst->lib_geom->chld;
  wst->lib_geom->chld = &geom->base;
  
  /* make instance geeometry and attach to the root node  */
  instGeom = ak_instanceMakeGeom(wst->heap, wst->node, wst->obj.geom);
  
  if (wst->node->geometry) {
    wst->node->geometry->base.prev = (void *)instGeom;
    instGeom->base.next            = &wst->node->geometry->base;
  }

  instGeom->base.next = (void *)wst->node->geometry;
  wst->node->geometry = instGeom;
  
  /* prepare buffers */
  
  /* move data to buffers */
  buff_p = ak_heap_calloc(heap, doc, sizeof(*buff_p));

  flist_sp_insert(&doc->lib.buffers, buff_p);

  buff_p->data   = ak_heap_alloc(heap, buff_p, wst->obj.dc_pos->usedsize);
  buff_p->length = wst->obj.dc_pos->usedsize;
  ak_data_join(wst->obj.dc_pos, buff_p->data);
  
  poly->base.indices = ak_heap_calloc(heap,
                                      poly,
                                      sizeof(*poly->base.indices)
                                      + wst->obj.dc_pos->usedsize);
  poly->base.indices->count = wst->obj.dc_indv->itemcount;
  ak_data_join(wst->obj.dc_indv, poly->base.indices->items);

  poly->vcount = ak_heap_calloc(heap,
                                poly,
                                sizeof(*poly->vcount)
                                + wst->obj.dc_vcount->usedsize);
  poly->vcount->count = wst->obj.dc_vcount->itemcount;
  ak_data_join(wst->obj.dc_vcount, poly->vcount->items);

  poly->base.indexStride = 1;
  poly->base.inputCount  = 1;
  poly->base.type        = AK_PRIMITIVE_POLYGONS;
  poly->base.count       = 6;

  acc_p                 = ak_heap_calloc(heap, doc, sizeof(*acc_p));
  acc_p->buffer         = buff_p;
  acc_p->byteLength     = buff_p->length;
  acc_p->byteStride     = sizeof(float) * 4;
  acc_p->componentSize  = AK_COMPONENT_SIZE_VEC3;
  acc_p->componentType  = AKT_FLOAT;
  acc_p->componentBytes = sizeof(float) * 3;
  acc_p->componentCount = 3;
  acc_p->fillByteSize   = sizeof(float) * 3;
  acc_p->count          = (uint32_t)wst->obj.dc_pos->itemcount;

  inp_p              = ak_heap_calloc(heap, poly, sizeof(*inp_p));
  inp_p->accessor    = acc_p;
  inp_p->semantic    = AK_INPUT_SEMANTIC_POSITION;
  inp_p->semanticRaw = ak_heap_strdup(heap, inp_p, "POSITION");

  poly->base.input = inp_p;
  poly->base.pos   = inp_p;
}

void
wobj_switchObject(WOState * __restrict wst) {
  AkGeometry *geom;
  AkMesh     *mesh;
  AkPolygon  *poly;
  
  wobj_finishObject(wst);

  mesh = ak_allocMesh(wst->heap, wst->lib_geom, &geom);
  poly = ak_heap_calloc(wst->heap, ak_objFrom(mesh), sizeof(*poly));
  
  poly->base.type      = AK_PRIMITIVE_POLYGONS;
  poly->base.mesh      = mesh;

  mesh->primitive      = (AkMeshPrimitive *)poly;
  mesh->primitiveCount = 1;
  
  /* set current geometry */
  wst->obj.geom = geom;
  
  /* vertex index */
  wst->obj.dc_indv = ak_data_new(wst->doc, 128, sizeof(AkUInt), NULL);

  /* vertex data */
  wst->obj.dc_pos    = ak_data_new(wst->doc, 128, sizeof(vec4), ak_cmp_vec4);
  wst->obj.dc_tex    = ak_data_new(wst->doc, 128, sizeof(vec3), ak_cmp_vec3);
  wst->obj.dc_nor    = ak_data_new(wst->doc, 128, sizeof(vec3), ak_cmp_vec3);
  wst->obj.dc_vcount = ak_data_new(wst->doc, 128, sizeof(int32_t), ak_cmp_i32);
}

void
wobj_switchGroup(WOState * __restrict wst) {
  
}

void
wobj_finishGroup(WOState * __restrict wst) {
  
}

AkResult _assetkit_hide
wobj_obj(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  AkHeap             *heap;
  AkDoc              *doc;
  void               *objstr;
  char               *p;
  AkLibrary          *lib_geom, *lib_vscene;
  AkVisualScene      *scene;
  WOState             wstVal = {0}, *wst;
  float               v[4];
  size_t              objstrSize;
  AkResult            ret;
  size_t              faceSize;
  char                c;

  if ((ret = ak_readfile(filepath, "rb", &objstr, &objstrSize)) != AK_OK)
    return ret;

  c    = '\0';
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf                = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name          = filepath;
  doc->inf->dir           = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage     = true;
  doc->inf->ftype         = AK_FILE_TYPE_WAVEFRONT;
  doc->inf->base.coordSys = AK_YUP;
  doc->coordSys           = AK_YUP; /* Default */

  if (!((p = objstr) && (c = *p) != '\0')) {
    ak_free(doc);
    return AK_ERR;
  }
  
  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  ((RBTree *)doc->reserved)->onFreeNode = ak_wobjFreeDupl;
  
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
  memset(&wstVal, 0, sizeof(wstVal));
  wst              = &wstVal;
  wstVal.doc       = doc;
  wstVal.heap      = heap;
  wstVal.tmpParent = ak_heap_alloc(heap, doc, sizeof(void*));
  wstVal.node      = scene->node;
  wstVal.lib_geom  = doc->lib.geometries;

  /* parse .obj */
  do {
    /* skip spaces */
    SKIP_SPACES

    if (p[1] == ' ' || p[1] == '\t') {
      switch (c) {
        case '#': {
          /* ignore comments */
          while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
          /* while ((c = *++p) != '\0' &&  AK_ARRAY_NLINE_CHECK); */
          break;
        }
        case 'v': {
          if (*++p == '\0')
            goto err;

          ak_strtof_line(p, 0, 4, v);
          ak_data_append(wst->obj.dc_pos, v);
          break;
        }
        case 'f': {
          if ((c = *(p += 2)) == '\0')
            goto err;

          faceSize = 0;
          do {
            char   *endp;
            int32_t indv;

            SKIP_SPACES
            
            indv = (int32_t)strtol(p, &endp, 10);
            
            /* TODO: handle negative indices */
            if (indv > 0)
              indv--;

            faceSize += 1;

            ak_data_append(wst->obj.dc_indv, &indv);
          } while (p
                   && p[0] != '\0'
                   && (c = *++p) != '\0'
                   && !AK_ARRAY_NLINE_CHECK);
          
          ak_data_append(wst->obj.dc_vcount, &faceSize);
          break;
        }
        case 'o': {
          wobj_switchObject(wst);
          break;
        }
        case 'g': {
          wobj_switchGroup(wst);
          break;
        }
        default:
          break;
      }
    }
    
    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  wobj_finishObject(wst);
  wobj_finishGroup(wst);
  
  wobj_postscript(wst);

  *dest = doc;

  return AK_OK;

err:
  ak_free(doc);
  return AK_ERR;
}

static
void
ak_wobjFreeDupl(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}
