/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_util.h"
#include "../ak_common.h"

_assetkit_hide
uint32_t
ak_meshTriangulatePoly_noindices(AkPolygon * __restrict poly);

/* not tested yet! */
_assetkit_hide
uint32_t
ak_meshTriangulatePoly_noindices(AkPolygon * __restrict poly) {
  AkBuffer     *buff, *newbuff;
  AkHeap       *heap;
  AkUInt       *vc_it;
  AkAccessor   *acc;
  AkSource     *src;
  AkFloat      *it_new, *it_old;
  AkUInt        trianglec, otherc, i, st, isz;

  if (!poly->base.pos
      || !(src  = ak_getObjectByUrl(&poly->base.pos->base.source))
      || !(acc  = src->tcommon)
      || !(buff = ak_getObjectByUrl(&acc->source)))
    return 0;

  otherc    = 0;
  trianglec = 0;
  vc_it     = poly->vcount->items;
  for (i = 0; i < poly->vcount->count; i++) {
    if (vc_it[i] > 3)
      trianglec += vc_it[i] - 2;
    else
      otherc += vc_it[i];
  }

  if (trianglec == 0)
    return 0;

  isz  = sizeof(AkFloat);
  st   = acc->stride;
  heap = ak_heap_getheap(poly->vcount);

  newbuff = ak_heap_calloc(heap, poly, sizeof(*newbuff));
  newbuff->data = ak_heap_alloc(heap,
                                newbuff,
                                isz * trianglec * 3 * st);

  newbuff->length = isz * trianglec * 3 * st;

  it_old = newbuff->data;
  it_new = buff->data;

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

