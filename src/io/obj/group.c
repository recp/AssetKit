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
#include "../../strpool.h"

/* Buffer > Accessor > Input > Prim > Mesh > Geom > InstanceGeom > Node */

AK_HIDE
static void
wobj_finishPrim(WOState  * __restrict wst,
                WOObject * __restrict wo,
                WOPrim   * __restrict wp) {
  AkHeap             *heap;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  uint32_t            inputOffset;

  if (wp->maxVC == 0)
    return;

  heap        = wst->heap;
  geom        = wo->geom;
  mesh        = ak_objGet(geom->gdata);
  inputOffset = 0;

  /* finish prim */
  if (wp->maxVC == 3) {
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
                                  + wp->dc_vcount->usedsize);
    poly->vcount->count = wp->dc_vcount->itemcount;
    ak_data_join(wp->dc_vcount, poly->vcount->items, 0, 0);
    
    prim = (AkMeshPrimitive *)poly;
  }
  
  prim->mesh      = mesh;
  prim->next      = mesh->primitive;
  mesh->primitive = prim;
  mesh->primitiveCount++;
  
  prim->pos = wobj_input(wst, prim, wst->ac_pos,
                         AK_INPUT_POSITION, _s_POSITION, inputOffset++);
  
  if (wst->mtlib && wp->mtlname)
    prim->material = rb_find(wst->mtlib->materials, (void *)wp->mtlname);
  
   if (wst->dc_tex->itemcount > 0)
     wobj_input(wst, prim, wst->ac_tex,
                AK_INPUT_TEXCOORD, _s_TEXCOORD, inputOffset++);
 
   if (wst->dc_nor->itemcount > 0)
     wobj_input(wst, prim, wst->ac_nor,
                AK_INPUT_NORMAL, _s_NORMAL, inputOffset);
   
  /* fix indices */
  wobj_joinIndices(wst, wp, prim);
}

AK_HIDE
WOPrim*
wobj_switchPrim(WOState * __restrict wst, const char * __restrict mtlname) {
  WOPrim *wp;

  if ((wp = wst->obj->prim) && wp->dc_face->itemcount == 0) {
    wp->mtlname = mtlname;
    return wst->obj->prim;
  }

  wp             = ak_heap_calloc(wst->heap, wst->tmp, sizeof(*wp));
  wp->dc_face    = ak_data_new(wst->tmp, 128, sizeof(ivec3), ak_cmp_ivec3);
  wp->dc_vcount  = ak_data_new(wst->tmp, 128, sizeof(int32_t), NULL);
  wp->mtlname    = mtlname;
  wp->next       = wst->obj->prim;
  wst->obj->prim = wp;

  return wp;
}

AK_HIDE
void
wobj_finishObject(WOState * __restrict wst, WOObject * __restrict obj) {
  WOPrim             *wp, *next;
  AkInstanceGeometry *instGeom;
  AkGeometry         *geom;
  
  if (!obj->geom)
    return;
  
  /* clean the geom if none resource is found for default state */
  if (wst->dc_pos->itemcount < 1)
    return;

  geom = obj->geom;

  /* add to library */
  geom->base.next     = wst->lib_geom->chld;
  wst->lib_geom->chld = &geom->base;
  wst->lib_geom->count++;
  
  /* make instance geeometry and attach to the root node  */
  instGeom = ak_instanceMakeGeom(wst->heap, wst->node, geom);
  
  if (wst->node->geometry) {
    wst->node->geometry->base.prev = (void *)instGeom;
    instGeom->base.next            = (void *)wst->node->geometry;
  }

  wst->node->geometry = instGeom;

  /* mesh primitives */
  wp = obj->prim;
  do {
    next = wp->next;
    wobj_finishPrim(wst, obj, wp);
  } while ((wp = next));
}

AK_HIDE
void
wobj_finishObjects(WOState * __restrict wst) {
  WOObject *obj;

  obj = wst->obj;
  while (obj) {
    wobj_finishObject(wst, obj);
    obj = obj->next;
  }
}

AK_HIDE
void
wobj_switchObject(WOState * __restrict wst) {
  WOObject   *obj;
  AkGeometry *geom;
  
  if (wst->obj && wst->obj->prim && wst->obj->prim->dc_face->itemcount == 0)
    return;

  obj       = ak_heap_calloc(wst->heap, wst->tmp, sizeof(*obj));
  obj->next = wst->obj;
  wst->obj  = obj;
  
  ak_allocMesh(wst->heap, wst->lib_geom, &geom);

  /* set current geometry */
  obj->geom = geom;

  wobj_switchPrim(wst, NULL);
}
