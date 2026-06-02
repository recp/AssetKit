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
#include "../../mat/internal.h"

/* Buffer > Accessor > Input > Prim > Mesh > Geom > InstanceGeom > Node */

static
void
wobj_finishPrim(WOState  * __restrict wst,
                WOObject * __restrict wo,
                WOPrim   * __restrict wp);

static
void
wobj_addPointFallback(WOState * __restrict wst, WOObject * __restrict obj);

static
void
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
  if (wp->kind == AK_PRIMITIVE_LINES) {
    AkLines *lines;

    lines            = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*lines));
    lines->base.type = AK_PRIMITIVE_LINES;
    lines->mode      = AK_LINES;
    prim = (AkMeshPrimitive *)lines;
  } else if (wp->kind == AK_PRIMITIVE_POINTS) {
    prim       = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*prim));
    prim->type = AK_PRIMITIVE_POINTS;
  } else if (wp->maxVC == 3) {
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

    poly->vcount->count  = wp->dc_vcount->itemcount;
    poly->base.nPolygons = (uint32_t)poly->vcount->count;

    ak_data_join(wp->dc_vcount, poly->vcount->items, 0, 0);

    prim = (AkMeshPrimitive *)poly;
  }

  prim->mesh      = mesh;
  prim->next      = mesh->primitive;
  mesh->primitive = prim;
  mesh->primitiveCount++;
  if (wp->smooth)
    prim->reserved1 |= WOBJ_PRIM_FLAG_SMOOTH;
  
  if (wst->mtlib && wp->mtlname)
    prim->material = rb_find(wst->mtlib->materials, (void *)wp->mtlname);

  if (!prim->material && wst->ac_col)
    prim->material = ak_materialDefaultVertexColorAlpha(wst->doc, wst->hasColorAlpha);

  if (wp->kind != AK_PRIMITIVE_LINES
      && wp->kind != AK_PRIMITIVE_POINTS
      && wp->maxVC == 3
      && wobj_flattenPrimDirect(wst, wp, prim)) {
    prim->nPolygons = (uint32_t)wp->dc_face->itemcount / 3;
    return;
  }

  if ((wp->kind == AK_PRIMITIVE_LINES || wp->kind == AK_PRIMITIVE_POINTS)
      && wobj_compactIndexedPointPrim(wst, wp, prim)) {
    if (wp->kind == AK_PRIMITIVE_LINES)
      prim->nPolygons = (uint32_t)prim->indices->count / 2;
    else
      prim->nPolygons = (uint32_t)prim->indices->count;
    return;
  }

  prim->pos = wobj_input(wst, prim, wst->ac_pos,
                         AK_INPUT_POSITION, _s_POSITION, inputOffset++);

  if (wst->ac_col)
    wobj_input(wst, prim, wst->ac_col, AK_INPUT_COLOR, _s_COLOR, 0);
  
   if (wp->hasTexture && wst->dc_tex->itemcount > 0)
     wobj_input(wst, prim, wst->ac_tex,
                AK_INPUT_TEXCOORD, _s_TEXCOORD, inputOffset++);
 
   if (wp->hasNormal && wst->dc_nor->itemcount > 0)
     wobj_input(wst, prim, wst->ac_nor,
                AK_INPUT_NORMAL, _s_NORMAL, inputOffset);
   
  /* fix indices */
  wobj_joinIndices(wst, wp, prim);

  if (wp->kind == AK_PRIMITIVE_LINES) {
    prim->nPolygons = (uint32_t)prim->indices->count / 2;
  } else if (wp->kind == AK_PRIMITIVE_POINTS) {
    prim->nPolygons = (uint32_t)prim->indices->count;
  } else if (wp->maxVC == 3) {
    prim->nPolygons = (uint32_t)prim->indices->count
                      / (3u * prim->indexStride);
  }
}

AK_HIDE
WOPrim*
wobj_switchPrim(WOState * __restrict wst, const char * __restrict mtlname) {
  WOPrim *wp;

  if ((wp = wst->obj->prim) && wp->dc_face->itemcount == 0) {
    wp->mtlname = mtlname;
    wp->smooth  = wst->smooth;
    return wst->obj->prim;
  }

  wp             = ak_heap_calloc(wst->heap, wst->tmp, sizeof(*wp));
  wp->dc_face    = ak_data_new(wst->tmp,
                               WOBJ_DATA_NODE_ITEMS,
                               sizeof(ivec3),
                               ak_cmp_ivec3);
  wp->dc_vcount  = ak_data_new(wst->tmp,
                               WOBJ_DATA_NODE_ITEMS,
                               sizeof(int32_t),
                               NULL);
  wp->mtlname    = mtlname;
  wp->smooth     = wst->smooth;
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
  AK_LIB_PREPEND(*wst->lib_geom, geom, next);
  
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

static
void
wobj_addPointFallback(WOState * __restrict wst, WOObject * __restrict obj) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;

  if (!obj || !obj->geom || !wst->ac_pos || wst->ac_pos->count == 0)
    return;

  mesh = ak_objGet(obj->geom->gdata);
  if (!mesh || mesh->primitive)
    return;

  prim              = ak_heap_calloc(wst->heap, ak_objFrom(mesh), sizeof(*prim));
  prim->type        = AK_PRIMITIVE_POINTS;
  prim->mesh        = mesh;
  prim->indexStride = 1;
  prim->nPolygons   = wst->ac_pos->count;
  prim->pos         = wobj_input(wst,
                                 prim,
                                 wst->ac_pos,
                                 AK_INPUT_POSITION,
                                 _s_POSITION,
                                 0);
  mesh->primitive   = prim;
  mesh->primitiveCount++;
}

AK_HIDE
void
wobj_finishObjects(WOState * __restrict wst) {
  WOObject *obj;
  WOObject *fallbackObj;
  bool      hasPrimitive;

  obj = wst->obj;
  fallbackObj = NULL;
  hasPrimitive = false;
  while (obj) {
    AkMesh *mesh;

    if (!fallbackObj && obj->geom)
      fallbackObj = obj;

    wobj_finishObject(wst, obj);
    if (obj->geom
        && (mesh = ak_objGet(obj->geom->gdata))
        && mesh->primitive)
      hasPrimitive = true;

    obj = obj->next;
  }

  if (!hasPrimitive && !wst->hasFreeform)
    wobj_addPointFallback(wst, fallbackObj);
}

AK_HIDE
void
wobj_switchObject(WOState * __restrict wst) {
  WOObject   *obj;
  AkGeometry *geom;

  wst->smooth = false;
  
  if (wst->obj && wst->obj->prim && wst->obj->prim->dc_face->itemcount == 0) {
    wst->obj->prim->mtlname = wst->mtlname;
    wst->obj->prim->smooth = false;
    return;
  }

  obj       = ak_heap_calloc(wst->heap, wst->tmp, sizeof(*obj));
  obj->next = wst->obj;
  wst->obj  = obj;
  
  ak_allocMesh(wst->heap, wst->doc, &geom);

  /* set current geometry */
  obj->geom = geom;

  wobj_switchPrim(wst, wst->mtlname);
}
