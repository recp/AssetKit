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
#include "../../../topo/topo.h"

static
void
dae_mesh_needs_edit(AkMesh * __restrict mesh,
                    bool                 checkTriangulate,
                    bool                 checkNormals,
                    bool              * __restrict needsTriangulate,
                    bool              * __restrict needsNormals,
                    bool              * __restrict needsFixIndices) {
  AkMeshPrimitive *prim;
  AkAccessor      *acc;
  AkInput         *input;
  bool             foundNormal;

  *needsTriangulate = false;
  *needsNormals     = false;
  *needsFixIndices  = false;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (checkTriangulate
        && !*needsTriangulate
        && prim->type == AK_PRIMITIVE_POLYGONS)
      *needsTriangulate = true;

    if (!*needsFixIndices
        && (prim->indices || prim->indexAccessor)
        && prim->indexStride > 1)
      *needsFixIndices = true;

    if (checkNormals
        && !*needsNormals
        && (prim->type == AK_PRIMITIVE_TRIANGLES
            || prim->type == AK_PRIMITIVE_POLYGONS)) {
      foundNormal = false;
      input       = prim->input;

      while (input) {
        if (input->semantic == AK_INPUT_NORMAL) {
          foundNormal = true;
          acc         = input->accessor;

          if (!acc || !acc->buffer)
            *needsNormals = true;

          break;
        }

        input = input->next;
      }

      if (!foundNormal)
        *needsNormals = true;
    }

    if ((!checkTriangulate || *needsTriangulate)
        && (!checkNormals || *needsNormals)
        && *needsFixIndices)
      break;
  }
}

AK_HIDE
AkResult
dae_mesh_fixup(AkMesh * mesh, bool retainDuplicators) {
  AkMeshEditHelper *edith;
  AkHeap           *heap;
  AkDoc            *doc;
  bool              needsTriangulate, needsNormals, needsFixIndices;

  heap = ak_heap_getheap(mesh->geom);
  doc  = ak_heap_data(heap);

  topofix(mesh);

  /* first fixup coord system because verts will be duplicated,
     reduce extra process */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
      && (void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
    ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));

  if (!mesh->primitive)
    return AK_OK;

  dae_mesh_needs_edit(mesh,
                      ak_opt_get(AK_OPT_TRIANGULATE),
                      ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED),
                      &needsTriangulate,
                      &needsNormals,
                      &needsFixIndices);

  if (!needsTriangulate && !needsNormals && !needsFixIndices) {
    if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
      ak_bbox_mesh(mesh);
    return AK_OK;
  }

  ak_meshBeginEdit(mesh);

  edith                 = mesh->edith;
  edith->skipFixIndices = true; /* to do it once per mesh */

  if (needsTriangulate)
    ak_meshTriangulate(mesh);

  if (needsNormals)
    ak_meshGenNormals(mesh);

  edith->skipFixIndices = false;
  if (needsFixIndices || needsNormals)
    ak_meshFixIndicesDefaultRetainDuplicators(mesh,
                                              retainDuplicators
                                              || mesh->skins != NULL);

  ak_meshEndEdit(mesh);

  if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
    ak_bbox_mesh(mesh);
  
  return AK_OK;
}
