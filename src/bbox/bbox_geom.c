/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "bbox.h"

void
ak_bbox_geom(struct AkGeometry * __restrict geom) {
  AkObject *primitive;

  primitive = geom->gdata;
  switch ((AkGeometryType)primitive->type) {
    case AK_GEOMETRY_MESH:
      ak_bbox_mesh(ak_objGet(primitive));
      break;
    case AK_GEOMETRY_SPLINE: /* TODO: */
    case AK_GEOMETRY_BREP: /* TODO: */
      break;
  }
}
