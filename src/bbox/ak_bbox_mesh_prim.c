/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_bbox.h"
#include "../mesh/ak_mesh_util.h"
#include <cglm/cglm.h>
#include <float.h>

void
ak_bbox_mesh_prim(struct AkMeshPrimitive * __restrict prim) {
  AkHeap     *heap;
  AkGeometry *geom;
  AkMesh     *mesh;
  AkBuffer   *posbuff;
  char       *data;
  AkSource   *src;
  AkAccessor *acc;
  vec3        min, max;

  mesh    = prim->mesh;
  geom    = mesh->geom;
  posbuff = NULL;
  acc     = NULL;

  if (!prim->pos)
    return;

  if (!(src = ak_getObjectByUrl(&prim->pos->base.source))
      || !(acc = src->tcommon)
      || !(posbuff = ak_getObjectByUrl(&acc->source)))
    return;

  data = ((char *)posbuff->data + acc->byteOffset);

  glm_vec_broadcast(FLT_MAX, min);
  glm_vec_broadcast(-FLT_MAX, max);

  /* we must walk through indices if exists because source may contain
     unrelated data and this will cause get wrong box
   */
  if (prim->indices) {
    AkUInt  *ind;
    size_t   i, icount;
    uint32_t st, vo;

    icount = prim->indices->count;
    vo     = ak_mesh_vertex_off(prim);
    st     = prim->indexStride;
    ind    = prim->indices->items;

    for (i = 0; i < icount; i += st) {
      float *vec;
      vec = (float *)(data + ind[i + vo] * acc->byteStride);
      ak_bbox_pick(min, max, vec);
    }
  } else {
    size_t i;
    for (i = 0; i < acc->count; i++) {
      float *vec;
      vec = (float *)(data + acc->byteStride * i);
      ak_bbox_pick(min, max, vec);
    }
  }

  heap = ak_heap_getheap(prim);

  if (!prim->bbox)
    prim->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));

  if (!mesh->bbox)
    mesh->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));

  if (!geom->bbox)
    geom->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));

  glm_vec_copy(min, prim->bbox->min);
  glm_vec_copy(max, prim->bbox->max);

  ak_bbox_pick_pbox(mesh->bbox, prim->bbox);
  ak_bbox_pick_pbox(geom->bbox, mesh->bbox);
}
