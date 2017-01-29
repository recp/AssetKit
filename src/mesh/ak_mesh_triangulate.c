/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_util.h"
#include "../ak_common.h"

/* not tested yet! */
AK_EXPORT
uint32_t
ak_meshTriangulatePoly_noindices(AkPolygon * __restrict poly) {
  AkSourceFloatArray *arr, *newarr;
  AkHeap             *heap;
  AkUInt             *vc_it;
  AkAccessor         *acc;
  AkSource           *src;
  AkInputBasic       *inputb;
  AkObject           *data, *newdata;
  AkFloat            *it_new, *it_old;
  AkUInt              trianglec, otherc, i, st, isz;

  otherc    = 0;
  trianglec = 0;
  vc_it     = poly->vcount->items;
  for (i = 0; i < poly->vcount->count; i++) {
    if (vc_it[i] > 3)
      trianglec += vc_it[i] - 2;
    else
      otherc += vc_it[i];
  }

  if (!trianglec)
    return trianglec;

  if (!poly->base.vertices)
    return 0;

  src = NULL;
  acc = NULL;
  arr = NULL;

  inputb = poly->base.vertices->input;
  while (inputb) {
    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
      src = ak_getObjectByUrl(&inputb->source);
      if (!src)
        return 0;

      acc = src->tcommon;
      if (!acc)
        return 0;

      data = ak_getObjectByUrl(&acc->source);
      if (!data)
        return 0;

      arr = ak_objGet(data);
    }
    inputb = inputb->next;
  }

  if (!src || !acc || !arr)
    return 0;

  isz  = sizeof(AkFloat);
  st   = acc->stride;
  heap = ak_heap_getheap(poly->vcount);

  newdata = ak_heap_alloc(heap,
                          poly,
                          sizeof(*newdata)
                          + isz * (arr->count
                                   + (trianglec - 1) * 3 * st));
  newarr = ak_objGet(newdata);
  newarr->count = arr->count + (trianglec - 1) * 3 * st;

  it_old  = newarr->items;
  it_new = arr->items;

  for (i = 0; i < poly->vcount->count; i++) {
    uint32_t vc, j;

    vc = vc_it[i];
    if (vc > 2) {
      for (j = 1; j < vc - 1; j++) {
        memcpy(it_new, it_old, st * isz);
        it_new += st;

        memcpy(it_new,
               it_old + j * st,
               2 * st * isz);
        it_new += 2 * st;
      }
    } else {
      memcpy(it_new, it_old, vc * st * isz);
      it_new += vc * st;
    }

    it_old += vc * st;
  }

  return trianglec;
}

AK_EXPORT
uint32_t
ak_meshTriangulatePoly(AkPolygon * __restrict poly) {
  AkHeap      *heap;
  AkUIntArray *newind;
  AkUInt      *vc_it, *ind_it, *newind_it;
  AkUInt       trianglec, otherc, i, st;
  AkUInt       isz;

  if (!poly->vcount)
    return 0;

  /* we have only primitives for direct draw, no indexes :( */
  if (!poly->base.indices)
    return ak_meshTriangulatePoly_noindices(poly);

  otherc    = 0;
  trianglec = 0;
  vc_it     = poly->vcount->items;
  for (i = 0; i < poly->vcount->count; i++) {
    if (vc_it[i] > 3)
      trianglec += vc_it[i] - 2;
    else
      otherc += vc_it[i];
  }

  if (!trianglec)
    return trianglec;

  isz    = sizeof(AkUInt);
  heap   = ak_heap_getheap(poly->vcount);
  ind_it = poly->base.indices->items;
  st     = poly->base.indexStride;
  newind = ak_heap_alloc(heap,
                         poly,
                         sizeof(*newind)
                         + isz * (otherc + trianglec * 3) * st);
  newind->count = (otherc + trianglec * 3) * st;
  newind_it     = newind->items;

  for (i = 0; i < poly->vcount->count; i++) {
    uint32_t vc, j;

    vc = vc_it[i];
    if (vc > 2) {
      for (j = 1; j < vc - 1; j++) {
        memcpy(newind_it, ind_it, st * isz);
        newind_it += st;

        memcpy(newind_it,
               ind_it + j * st,
               2 * st * isz);
        newind_it += 2 * st;
      }
    } else {
      memcpy(newind_it, ind_it, vc * st * isz);
      newind_it += vc * st;
    }

    ind_it += vc * st;
  }

  ak_free(poly->base.indices);
  poly->base.indices = newind;

  /* no need to this info anymore, save space! */
  ak_free(poly->vcount);
  poly->vcount = NULL;

  return trianglec;
}

AK_EXPORT
uint32_t
ak_meshTriangulate(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  uint32_t         extc;

  extc = 0;
  prim = mesh->primitive;
  while (prim) {
    if (prim->type == AK_MESH_PRIMITIVE_TYPE_POLYGONS)
      extc += ak_meshTriangulatePoly((AkPolygon *)prim);

    prim = prim->next;
  }
  return extc;
}
