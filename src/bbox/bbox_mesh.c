/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "bbox.h"

void
ak_bbox_mesh(struct AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  vec3             center;
  int32_t          primcount;

  primcount = 0;
  prim      = mesh->primitive;

  while (prim) {
    ak_bbox_mesh_prim(prim);
    primcount++;
    prim = prim->next;
  }

  /* compute centroid */

  if (!ak_opt_get(AK_OPT_COMPUTE_EXACT_CENTER)) {
    glm_vec_center(mesh->bbox->min,
                   mesh->bbox->max,
                   mesh->center);
  } else if (primcount > 0) {
    /* calculate exact center of primitive */
    glm_vec_divs(center, primcount, mesh->center);
  } else {
    glm_vec_zero(mesh->center);
  }
}
