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
  AkMeshPrimitive *prim;
  vec3             center;
  int32_t          primcount;

  primcount = 0;
  prim      = mesh->primitive;

  if (!mesh->bbox)
    mesh->bbox = ak_heap_calloc(ak_heap_getheap(prim),
                                ak_objFrom(mesh),
                                sizeof(*mesh->bbox));
  
  while (prim) {
    ak_bbox_mesh_prim(prim);
    primcount++;
    prim = prim->next;
  }

  /* compute centroid */

  if (!ak_opt_get(AK_OPT_COMPUTE_EXACT_CENTER)) {
    ak_bbox_center(mesh->bbox, mesh->center);
  } else {
    glm_vec3_zero(center);

    /* calculate exact center of primitive */
    if (primcount > 0) {
      while (prim) {
        ak_bbox_mesh_prim(prim);

        glm_vec3_add(prim->center, center, center);
        primcount++;
        prim = prim->next;
      }
      glm_vec3_divs(center, (float)primcount, mesh->center);
    }
  }
}
