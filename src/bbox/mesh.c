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

void
ak_bbox_mesh(struct AkMesh * __restrict mesh) {
  AkHeap          *heap;
  AkMeshPrimitive *prim;
  vec3             center;
  int32_t          primcount, validPrimCount;

  if (!mesh)
    return;

  primcount = 0;
  prim      = mesh->primitive;
  heap      = ak_heap_getheap(ak_objFrom(mesh));

  if (!mesh->bbox)
    mesh->bbox = ak_heap_calloc(heap,
                                ak_objFrom(mesh),
                                sizeof(*mesh->bbox));

  ak_bbox_invalidate(mesh->bbox);
  if (mesh->geom && mesh->geom->bbox)
    ak_bbox_invalidate(mesh->geom->bbox);
  
  while (prim) {
    ak_bbox_mesh_prim(prim);
    primcount++;
    prim = prim->next;
  }

  /* compute centroid */

  if (!ak_opt_get(AK_OPT_COMPUTE_EXACT_CENTER)) {
    if (mesh->bbox->isvalid)
      ak_bbox_center(mesh->bbox, mesh->center);
    else
      glm_vec3_zero(mesh->center);
  } else {
    glm_vec3_zero(center);

    /* calculate exact center of primitive */
    if (primcount > 0) {
      validPrimCount = 0;
      prim = mesh->primitive;
      while (prim) {
        if (prim->bbox && prim->bbox->isvalid) {
          glm_vec3_add(prim->center, center, center);
          validPrimCount++;
        }
        prim = prim->next;
      }
      if (validPrimCount > 0)
        glm_vec3_divs(center, (float)validPrimCount, mesh->center);
    }
  }
}
