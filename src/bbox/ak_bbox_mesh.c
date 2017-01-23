/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_bbox.h"

void
ak_bbox_mesh(struct AkVisualScene * __restrict scene,
             struct AkGeometry    * __restrict geom,
             struct AkMesh        * __restrict mesh) {
  AkMeshPrimitive *prim;

  prim = mesh->primitive;
  while (prim) {
    ak_bbox_mesh_prim(scene,
                      geom,
                      mesh,
                      prim);
    prim = prim->next;
  }
}
