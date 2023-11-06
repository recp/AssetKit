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

#include "../common.h"

AK_HIDE
uint32_t
ak_meshTriangulatePoly_noindices(AkPolygon * __restrict poly);

/* not tested yet! */
AK_HIDE
uint32_t
ak_meshTriangulatePoly_noindices(AkPolygon * __restrict poly) {
  AkBuffer     *buff, *newbuff;
  AkHeap       *heap;
  AkUInt       *vc_it;
  AkAccessor   *acc;
  AkFloat      *it_new, *it_old;
  AkUInt        nGenTrigs, nTrigs, i, isz;
  size_t        st;

  if (!poly->base.pos
      || !(acc  = poly->base.pos->accessor)
      || !(buff = acc->buffer))
    return 0;

  nTrigs    = 0;
  nGenTrigs = 0;
  vc_it     = poly->vcount->items;

  for (i = 0; i < poly->vcount->count; i++) {
    if (vc_it[i] > 3) { nGenTrigs += vc_it[i] - 2; }
    else              { nTrigs    += 1;            }
  }

  if (!nGenTrigs) return 0;

  isz  = sizeof(AkFloat);
  st   = acc->byteStride;
  heap = ak_heap_getheap(poly->vcount);

  newbuff         = ak_heap_calloc(heap, poly, sizeof(*newbuff));
  newbuff->data   = ak_heap_alloc(heap, newbuff, isz * (nTrigs + nGenTrigs) * 3 * st);
  newbuff->length = isz * nGenTrigs * 3 * st;

  it_old = newbuff->data;
  it_new = buff->data;

  for (i = 0; i < poly->vcount->count; i++) {
    uint32_t vc, j;

    vc = vc_it[i];
    if (vc > 2) {
      for (j = 1; j < vc - 1; j++) {
        memcpy(it_new, it_old, st * isz);
        it_new += st;

        memcpy(it_new, it_old + j * st, 2 * st * isz);
        it_new += 2 * st;
      }
    } else {
      memcpy(it_new, it_old, vc * st * isz);
      it_new += vc * st;
    }

    it_old += vc * st;
  }

  return nGenTrigs;
}

AK_EXPORT
uint32_t
ak_meshTriangulatePoly(AkPolygon * __restrict poly) {
  AkHeap      *heap;
  AkUIntArray *newind;
  AkUInt      *vc_it, *ind_it, *newind_it;
  AkUInt       nGenTrigs, nTrigs, i, st;
  AkUInt       isz;

  if (!poly->vcount)
    return 0;

  /* we have only primitives for direct draw, no indexes :( */
  if (!poly->base.indices)
    return ak_meshTriangulatePoly_noindices(poly);

  nTrigs    = 0;
  nGenTrigs = 0;
  vc_it     = poly->vcount->items;

  for (i = 0; i < poly->vcount->count; i++) {
    if (vc_it[i] > 3) { nGenTrigs += vc_it[i] - 2; }
    else              { nTrigs    += 1;           }
  }

  if (!nGenTrigs) return 0;

  isz    = sizeof(AkUInt);
  heap   = ak_heap_getheap(poly->vcount);
  ind_it = poly->base.indices->items;
  st     = poly->base.indexStride;
  newind = ak_heap_alloc(heap,
                         poly,
                         sizeof(*newind)
                         + isz * ((nTrigs + nGenTrigs) * 3) * st);
  newind->count = ((nTrigs + nGenTrigs) * 3) * st;
  newind_it     = newind->items;

  for (i = 0; i < poly->vcount->count; i++) {
    uint32_t vc, j;

    vc = vc_it[i];
    if (vc > 2) {
      for (j = 1; j < vc - 1; j++) {
        memcpy(newind_it, ind_it, st * isz);
        newind_it += st;

        memcpy(newind_it, ind_it + j * st, 2 * st * isz);
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
  poly->base.count   = nGenTrigs + nTrigs;

  /* no need to this info anymore, save space! */
  ak_free(poly->vcount);
  poly->vcount = NULL;

  return nGenTrigs;
}

AK_EXPORT
uint32_t
ak_meshTriangulate(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  uint32_t         extc;

  extc = 0;
  prim = mesh->primitive;
  while (prim) {
    if (prim->type == AK_PRIMITIVE_POLYGONS)
      extc += ak_meshTriangulatePoly((AkPolygon *)prim);

    prim = prim->next;
  }
  return extc;
}
