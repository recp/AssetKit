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
