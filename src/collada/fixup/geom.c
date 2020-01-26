/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "geom.h"
#include "mesh.h"

AkResult _assetkit_hide
dae_geom_fixup_all(AkDoc * doc) {
  AkLibItem  *geomLib;
  AkGeometry *geom;

  geomLib = doc->lib.geometries;
  while (geomLib) {
    geom = geomLib->chld;
    while (geom) {
      dae_geom_fixup(geom);
      geom = geom->next;
    }

    geomLib = geomLib->next;
  }

  return AK_OK;
}

AkResult _assetkit_hide
dae_geom_fixup(AkGeometry * geom) {
  AkObject *primitive;

  primitive = geom->gdata;
  switch ((AkGeometryType)primitive->type) {
    case AK_GEOMETRY_MESH:
      dae_mesh_fixup(ak_objGet(primitive));
    default:
      break;
  }

  return AK_OK;
}
