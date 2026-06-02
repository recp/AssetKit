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

#include "geom.h"
#include "mesh.h"

AK_HIDE
AkResult
dae_geom_fixup_all(AkDoc * doc, bool retainDuplicators) {
  AkGeometry *geom;

  for (geom = doc->lib.geometries.first; geom; geom = geom->next)
    dae_geom_fixup(geom, retainDuplicators);

  return AK_OK;
}

AK_HIDE
AkResult
dae_geom_fixup(AkGeometry * geom, bool retainDuplicators) {
  AkObject *primitive;

  primitive = geom->gdata;
  switch ((AkGeometryType)primitive->type) {
    case AK_GEOMETRY_MESH:
      dae_mesh_fixup(ak_objGet(primitive), retainDuplicators);
    default:
      break;
  }

  return AK_OK;
}
