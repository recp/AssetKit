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
  AkHeap       *heap;
  AkGeometry   *geom;
  AkMesh       *mesh;
  AkBuffer     *posbuff;
  AkBufferView *buffView;
  char         *data;
  AkSource     *src;
  AkAccessor   *acc;
  float        *vec;
  vec3          center, min, max;
  size_t        i, count, byteStride;
  bool          exactCenter;

  mesh    = prim->mesh;
  geom    = mesh->geom;
  posbuff = NULL;
  acc     = NULL;

  if (!prim->pos
      || !(src = ak_getObjectByUrl(&prim->pos->source))
      || !(acc = src->tcommon)
      || !(buffView = acc->bufferView)
      || !(posbuff = buffView->buffer))
     // || !(posbuff = ak_getObjectByUrl(&acc->source)))
    return;

  data = ((char *)posbuff->data + buffView->byteOffset + acc->byteOffset);

  glm_vec3_broadcast(FLT_MAX, min);
  glm_vec3_broadcast(-FLT_MAX, max);
  glm_vec3_broadcast(0.0f, center);

  exactCenter = ak_opt_get(AK_OPT_COMPUTE_EXACT_CENTER);
  byteStride  = buffView->byteStride;
  
  /* we must walk through indices if exists because source may contain
     unrelated data and this will cause get wrong box
   */
  if (prim->indices) {
    AkUInt  *ind;
    size_t   icount;
    uint32_t st, vo;

    icount     = prim->indices->count;
    vo         = prim->pos->offset;
    st         = prim->indexStride;
    ind        = prim->indices->items;
    count      = icount;
    

    if (!exactCenter) {
      for (i = 0; i < icount; i += st) {
        vec = (float *)(data + ind[i + vo] * byteStride);
        glm_vec3_add(vec, center, center);
        ak_bbox_pick(min, max, vec);
      }
    } else {
      for (i = 0; i < icount; i += st) {
        vec = (float *)(data + ind[i + vo] * byteStride);
        ak_bbox_pick(min, max, vec);
      }
    }
  } else {
    count = acc->count;
    if (!exactCenter) {
      for (i = 0; i < count; i++) {
        vec = (float *)(data + byteStride * i);
        ak_bbox_pick(min, max, vec);
      }
    } else {
      for (i = 0; i < count; i++) {
        vec = (float *)(data + byteStride * i);
        glm_vec3_add(vec, center, center);
        ak_bbox_pick(min, max, vec);
      }
    }
  }
  
  heap = ak_heap_getheap(prim);

  if (!prim->bbox)
    prim->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));

  if (!mesh->bbox)
    mesh->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));

  if (!geom->bbox)
    geom->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));

  glm_vec3_copy(min, prim->bbox->min);
  glm_vec3_copy(max, prim->bbox->max);

  ak_bbox_pick_pbox(mesh->bbox, prim->bbox);
  ak_bbox_pick_pbox(geom->bbox, mesh->bbox);

  /* compute centroid */
  if (!ak_opt_get(AK_OPT_COMPUTE_EXACT_CENTER)) {
    glm_vec3_center(prim->bbox->min, prim->bbox->max, prim->center);
  } else if (count > 0) {
    /* calculate exact center of primitive */
    glm_vec3_divs(center, count, center);
  } else {
    glm_vec3_zero(prim->center);
  }
}
