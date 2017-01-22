/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include <assert.h>

AK_EXPORT
AkResult
ak_meshSetMaterial(AkMeshPrimitive *prim,
                   const char      *material) {
  AkGeometry *geom;
  AkMap      *map;

  geom = prim->geom;

#ifdef DEBUG
  assert(geom && "set geometry for this primitive!");
  assert(geom->materialMap && "set materialMap for this geom!");
#endif

  map = geom->materialMap;

  /* TODO: remove first */
  ak_multimap_add(map,
                  (AkMeshPrimitive **)&prim,
                  sizeof(AkMeshPrimitive **),
                  (void *)material);

  prim->material = material;
  return AK_OK;
}
