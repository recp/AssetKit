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
    byteStride = acc->fillByteSize;

  /* Walk indices/accessors directly because source may contain unrelated
     vertices; materializing glTF index accessors here would break zero-copy. */
  if (prim->indices || prim->indexAccessor) {
    AkIndexArray *ind;
    AkAccessor   *iacc;
    char         *idata;
    size_t        icount;
    size_t        istride;
    uint32_t      st, vo;

    vo    = prim->pos->indexOffset;
    st    = prim->indexStride ? prim->indexStride : 1;
    ind   = prim->indices;
    iacc  = prim->indexAccessor;
    count = 0;

#define AK_BBOX_PICK_FOR_INDEX_TYPE(TYPE, SRC)                              \
    do {                                                                    \
      const TYPE *src_;                                                     \
                                                                            \
      src_ = (const TYPE *)(const void *)(SRC);                             \
      for (i = 0; i + vo < icount; i += st) {                               \
        vec = (float *)(data + (AkUInt)src_[i + vo] * byteStride);          \
        if (exactCenter)                                                    \
          glm_vec3_add(vec, center, center);                                \
        ak_bbox_pick(min, max, vec);                                        \
        count++;                                                            \
      }                                                                     \
    } while (0)

#define AK_BBOX_PICK_FOR_ACCESSOR_TYPE(TYPE, READ_VALUE)                    \
    do {                                                                    \
      TYPE idx_;                                                            \
                                                                            \
      for (i = 0; i + vo < icount; i += st) {                               \
        READ_VALUE;                                                         \
        vec = (float *)(data + (AkUInt)idx_ * byteStride);                  \
        if (exactCenter)                                                    \
          glm_vec3_add(vec, center, center);                                \
        ak_bbox_pick(min, max, vec);                                        \
        count++;                                                            \
      }                                                                     \
    } while (0)
    
    if (ind) {
      icount = ind->count;
      switch (ind->componentType) {
        case AKT_UBYTE:
          AK_BBOX_PICK_FOR_INDEX_TYPE(uint8_t, ind->items);
          break;
        case AKT_USHORT:
          AK_BBOX_PICK_FOR_INDEX_TYPE(uint16_t, ind->items);
          break;
        case AKT_UINT:
          AK_BBOX_PICK_FOR_INDEX_TYPE(AkUInt, ind->items);
          break;
        default:
          break;
      }
    } else if (iacc && iacc->buffer && iacc->buffer->data) {
      icount  = iacc->count;
      istride = iacc->byteStride ? iacc->byteStride : iacc->bytesPerComponent;
      idata   = ((char *)iacc->buffer->data) + iacc->byteOffset;

      switch (iacc->componentType) {
        case AKT_UBYTE:
          AK_BBOX_PICK_FOR_ACCESSOR_TYPE(uint8_t,
            idx_ = *(uint8_t *)(void *)(idata + (i + vo) * istride));
          break;
        case AKT_USHORT:
          AK_BBOX_PICK_FOR_ACCESSOR_TYPE(uint16_t,
            memcpy(&idx_, idata + (i + vo) * istride, sizeof(idx_)));
          break;
        case AKT_UINT:
          AK_BBOX_PICK_FOR_ACCESSOR_TYPE(uint32_t,
            memcpy(&idx_, idata + (i + vo) * istride, sizeof(idx_)));
          break;
        default:
          break;
      }
    }

#undef AK_BBOX_PICK_FOR_ACCESSOR_TYPE
#undef AK_BBOX_PICK_FOR_INDEX_TYPE
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
