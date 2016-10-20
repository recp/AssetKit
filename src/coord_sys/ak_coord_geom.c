/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

AK_EXPORT
void
ak_changeCoordSysGeom(AkGeometry * __restrict geom,
                      AkCoordSys * newCoordSys) {
  AkObject *primitive;

  primitive = geom->gdata;
  switch ((AkGeometryType)primitive->type) {
    case AK_GEOMETRY_TYPE_MESH:
      ak_changeCoordSysMesh(ak_objGet(primitive), newCoordSys);
      break;
    case AK_GEOMETRY_TYPE_SPLINE: /* TODO: */
    case AK_GEOMETRY_TYPE_BREP: /* TODO: */
      break;
  }
}
