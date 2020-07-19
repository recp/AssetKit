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

#include "group.h"
#include "util.h"

AK_HIDE
void
wobj_finishObject(WOState * __restrict wst) {
  AkHeap             *heap;
  AkInstanceGeometry *instGeom;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  uint32_t            inputOffset;
  
  if (!wst->obj.geom || !wst->obj.dc_pos)
    return;
  
  /* clean the geom if none resource is found for default state */
  if (wst->obj.isdefault && wst->obj.dc_pos->itemcount < 1)
    goto cle;

  heap        = wst->heap;
  geom        = wst->obj.geom;
  mesh        = ak_objGet(geom->gdata);
  inputOffset = 0;
  
  /* add to library */
  geom->base.next     = wst->lib_geom->chld;
  wst->lib_geom->chld = &geom->base;
  
  /* make instance geeometry and attach to the root node  */
  instGeom = ak_instanceMakeGeom(wst->heap, wst->node, geom);
  
  if (wst->node->geometry) {
    wst->node->geometry->base.prev = (void *)instGeom;
    instGeom->base.next            = &wst->node->geometry->base;
  }

  instGeom->base.next = (void *)wst->node->geometry;
  wst->node->geometry = instGeom;

  /* finish prim */
  if (wst->maxVC == 3) {
    AkTriangles *tri;
    
    tri = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*tri));
    tri->mode      = AK_TRIANGLES;
    tri->base.type = AK_PRIMITIVE_TRIANGLES;
    prim = (AkMeshPrimitive *)tri;
  } else {
    AkPolygon *poly;
    
    poly = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*poly));
    poly->base.type = AK_PRIMITIVE_POLYGONS;
    
    poly->vcount = ak_heap_calloc(heap,
                                  poly,
                                  sizeof(*poly->vcount)
                                  + wst->obj.dc_vcount->usedsize);
    poly->vcount->count = wst->obj.dc_vcount->itemcount;
    ak_data_join(wst->obj.dc_vcount, poly->vcount->items, 0, 0);
    
    prim = (AkMeshPrimitive *)poly;
  }
  
  prim->mesh      = mesh;
  prim->next      = mesh->primitive;
  mesh->primitive = prim;
  mesh->primitiveCount++;
  
  prim->pos =
  wobj_addInput(wst, wst->obj.dc_pos, prim, AK_INPUT_SEMANTIC_POSITION,
                "POSITION", AK_COMPONENT_SIZE_VEC3, AKT_FLOAT, inputOffset++);
  
  if (wst->mtlib && wst->obj.mtlname)
    prim->material = rb_find(wst->mtlib->materials, wst->obj.mtlname);
  
  if (wst->obj.dc_nor->itemcount > 0) {
    wobj_addInput(wst, wst->obj.dc_nor, prim, AK_INPUT_SEMANTIC_NORMAL,
                  "NORMAL", AK_COMPONENT_SIZE_VEC3, AKT_FLOAT, inputOffset++);
  }
  
  if (wst->obj.dc_tex->itemcount > 0) {
    wobj_addInput(wst, wst->obj.dc_tex, prim, AK_INPUT_SEMANTIC_TEXCOORD,
                  "TEXCOORD", AK_COMPONENT_SIZE_VEC2, AKT_FLOAT, inputOffset);
  }

  wobj_joinIndices(wst, prim);
  wobj_fixIndices(prim);

cle:
  /* cleanup */
  if (wst->obj.dc_indv) {
    ak_free(wst->obj.dc_indv);
    ak_free(wst->obj.dc_indt);
    ak_free(wst->obj.dc_indn);
    ak_free(wst->obj.dc_pos);
    ak_free(wst->obj.dc_tex);
    ak_free(wst->obj.dc_nor);
    ak_free(wst->obj.dc_vcount);
  }

  memset(&wst->obj, 0, sizeof(wst->obj));
}

AK_HIDE
void
wobj_switchObject(WOState * __restrict wst) {
  AkGeometry *geom;
  
  wobj_finishObject(wst);

  ak_allocMesh(wst->heap, wst->lib_geom, &geom);
  
  /* set current geometry */
  wst->obj.geom = geom;
  
  /* vertex index */
  wst->obj.dc_indv = ak_data_new(wst->tmpParent, 128, sizeof(int32_t), NULL);
  wst->obj.dc_indt = ak_data_new(wst->tmpParent, 128, sizeof(int32_t), NULL);
  wst->obj.dc_indn = ak_data_new(wst->tmpParent, 128, sizeof(int32_t), NULL);

  /* vertex data */
  wst->obj.dc_pos    = ak_data_new(wst->tmpParent, 128, sizeof(vec3),    NULL);
  wst->obj.dc_tex    = ak_data_new(wst->tmpParent, 128, sizeof(vec2),    NULL);
  wst->obj.dc_nor    = ak_data_new(wst->tmpParent, 128, sizeof(vec3),    NULL);
  wst->obj.dc_vcount = ak_data_new(wst->tmpParent, 128, sizeof(int32_t), NULL);
}
