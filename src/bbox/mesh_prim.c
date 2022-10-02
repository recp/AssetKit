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

#include "bbox.h"
#include <cglm/cglm.h>
#include <float.h>

void
ak_bbox_mesh_prim(struct AkMeshPrimitive * __restrict prim) {
  AkHeap     *heap;
  AkGeometry *geom;
  AkMesh     *mesh;
  AkBuffer   *posbuff;
  char       *data;
  AkAccessor *acc;
  float      *vec;
  vec3        center, min, max;
  size_t      i, count, byteStride;
  bool        exactCenter;

  mesh    = prim->mesh;
  geom    = mesh->geom;
  posbuff = NULL;
  acc     = NULL;

  if (!prim->pos
      || !(acc = prim->pos->accessor)
      || !(posbuff = acc->buffer))
    return;

  data = ((char *)posbuff->data + acc->byteOffset);

  glm_vec3_broadcast(FLT_MAX, min);
  glm_vec3_broadcast(-FLT_MAX, max);
  glm_vec3_broadcast(0.0f, center);

  exactCenter = ak_opt_get(AK_OPT_COMPUTE_EXACT_CENTER);
  byteStride  = acc->byteStride;

  if (byteStride == 0)
    byteStride = acc->componentBytes;
  
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

  if (!prim->bbox) {
    prim->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));
    ak_bbox_invalidate(prim->bbox);
  }

  if (!mesh->bbox) {
    mesh->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));
    ak_bbox_invalidate(mesh->bbox);
  }

  if (!geom->bbox) {
    geom->bbox = ak_heap_calloc(heap, prim, sizeof(*prim->bbox));
    ak_bbox_invalidate(geom->bbox);
  }

  glm_vec3_copy(min, prim->bbox->min);
  glm_vec3_copy(max, prim->bbox->max);

  ak_bbox_pick_pbox(mesh->bbox, prim->bbox);
  ak_bbox_pick_pbox(geom->bbox, mesh->bbox);

  /* compute centroid */
  if (!ak_opt_get(AK_OPT_COMPUTE_EXACT_CENTER)) {
    glm_vec3_center(prim->bbox->min, prim->bbox->max, prim->center);
  } else if (count > 0) {
    /* calculate exact center of primitive */
    glm_vec3_divs(center, (float)count, center);
  } else {
    glm_vec3_zero(prim->center);
  }
}
