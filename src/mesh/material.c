/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include <assert.h>

AK_EXPORT
AkResult
ak_meshSetMaterial(AkMeshPrimitive *prim,
                   const char      *material) {
  AkGeometry *geom;
  AkMap      *map;

  geom = prim->mesh->geom;

#ifdef DEBUG
  assert(geom && "set geometry for this primitive!");
  assert(geom->materialMap && "set materialMap for this geom!");
#endif

  map = geom->materialMap;

  /* TODO: remove first */
  ak_multimap_add(map, prim, (void *)material);

  prim->bindmaterial = material;
  return AK_OK;
}
