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
#include "../../../mesh/index.h"

AK_HIDE
AkResult
dae_mesh_fixup(AkMesh * mesh) {
  AkMeshEditHelper *edith;
  AkHeap           *heap;
  AkDoc            *doc;

  heap = ak_heap_getheap(mesh->geom);
  doc  = ak_heap_data(heap);

  /* first fixup coord system because verts will be duplicated,
     reduce extra process */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
      && (void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
    ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));

  if (!mesh->primitive)
    return AK_OK;

  ak_meshBeginEdit(mesh);

  edith                 = mesh->edith;
  edith->skipFixIndices = true; /* to do it once per mesh */

  if (ak_opt_get(AK_OPT_TRIANGULATE))
    ak_meshTriangulate(mesh);

  if (ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED))
    if (ak_meshNeedsNormals(mesh))
      ak_meshGenNormals(mesh);

  edith->skipFixIndices = false;
  ak_meshFixIndices(mesh);

  ak_meshEndEdit(mesh);

  if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
    ak_bbox_mesh(mesh);
  
  return AK_OK;
}
