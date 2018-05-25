/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "bbox.h"
#include "../mesh/mesh_util.h"
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
  vec3        center, min, max;
  size_t      count;

  mesh    = prim->mesh;
  geom    = mesh->geom;
  posbuff = NULL;
  acc     = NULL;

  if (!prim->pos
      || !(src = ak_getObjectByUrl(&prim->pos->source))
      || !(acc = src->tcommon)
      || !(posbuff = ak_getObjectByUrl(&acc->source)))
    return;

  data = ((char *)posbuff->data + acc->byteOffset);

  glm_vec_broadcast(FLT_MAX, min);
  glm_vec_broadcast(-FLT_MAX, max);
  glm_vec_broadcast(0.0f, center);

  /* we must walk through indices if exists because source may contain
     unrelated data and this will cause get wrong box
   */
  if (prim->indices) {
    AkUInt  *ind;
    size_t   i, icount;
    uint32_t st, vo;

    icount = prim->indices->count;
    vo     = prim->pos->offset;
    st     = prim->indexStride;
    ind    = prim->indices->items;
    count  = icount;

    for (i = 0; i < icount; i += st) {
      float *vec;
      vec = (float *)(data + ind[i + vo] * acc->byteStride);
      glm_vec_add(vec, center, center);
      ak_bbox_pick(min, max, vec);
    }
  } else {
    size_t i;

    count = acc->count;
    for (i = 0; i < acc->count; i++) {
      float *vec;
      vec = (float *)(data + acc->byteStride * i);
      glm_vec_add(vec, center, center);
      ak_bbox_pick(min, max, vec);
    }
  }

  /* calculate exact center of primitive */
  glm_vec_divs(center, count, center);
  glm_vec4(center, 1.0f, prim->center);

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
