/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_util.h"
#include "../ak_common.h"

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

