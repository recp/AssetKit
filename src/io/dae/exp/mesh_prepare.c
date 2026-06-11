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

#include "mesh.h"

#include <stdlib.h>

AK_HIDE
bool
dae_prepare_geometry_index_mode(DAEExpState * __restrict st,
                                AkGeometry  * __restrict geom) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;

  if (!st || !geom || st->indexMode != AK_DAE_EXPORT_INDEX_SINGLE)
    return true;

  if (!geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return true;

  mesh = ak_objGet(geom->gdata);
  if (!mesh)
    return false;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!prim->mesh)
      prim->mesh = mesh;
    if (ak_meshPrimitiveEnsureSingleIndex(prim) != AK_OK)
      return false;
  }

  return true;
}

AK_HIDE
bool
dae_prepare_extra_geometry(DAEExpState * __restrict st,
                           AkGeometry  * __restrict geom) {
  DAEExpGeometryRef *ref;

  if (!st || !geom)
    return false;

  if (!dae_prepare_geometry_index_mode(st, geom))
    return false;

  if (dae_map_index(st->geometries, geom) != UINT32_MAX)
    return true;

  if (st->geometryCount == UINT32_MAX)
    return false;

  ref = calloc(1, sizeof(*ref));
  if (!ref)
    return false;

  ref->geom = geom;
  rb_insert(st->geometries, geom, (void *)(uintptr_t)(++st->geometryCount));

  if (st->lastExtraGeometry)
    st->lastExtraGeometry->next = ref;
  else
    st->extraGeometries = ref;
  st->lastExtraGeometry = ref;

  return true;
}
