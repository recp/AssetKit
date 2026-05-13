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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static
bool
dae_mesh_needs_triangulate(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (prim->type == AK_PRIMITIVE_POLYGONS)
      return true;
  }

  return false;
}

static
bool
dae_mesh_needs_fix_indices(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (prim->indices && prim->indexStride > 1)
      return true;
  }

  return false;
}

static
bool
dae_mesh_profile_enabled(void) {
  const char *value;

  value = getenv("ASSETKIT_DAE_MESH_PROFILE");
  if (!value)
    value = getenv("ASSETKIT_DAE_PROFILE");

  return value && value[0] && value[0] != '0';
}

static
double
dae_mesh_profile_now_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

AK_HIDE
AkResult
dae_mesh_fixup(AkMesh * mesh) {
  AkMeshEditHelper *edith;
  AkHeap           *heap;
  AkDoc            *doc;
  double            t0, t1, tBegin, tTriangulate, tNormals, tFixIndices,
                    tEnd, tBBox;
  bool              profile, needsTriangulate, needsNormals, needsFixIndices;

  heap = ak_heap_getheap(mesh->geom);
  doc  = ak_heap_data(heap);
  profile = dae_mesh_profile_enabled();
  t0 = profile ? dae_mesh_profile_now_ms() : 0.0;

  topofix(mesh);

  /* first fixup coord system because verts will be duplicated,
     reduce extra process */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
      && (void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
    ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));

  if (!mesh->primitive)
    return AK_OK;

  needsTriangulate = ak_opt_get(AK_OPT_TRIANGULATE)
                     && dae_mesh_needs_triangulate(mesh);
  needsNormals     = ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED)
                     && ak_meshNeedsNormals(mesh);
  needsFixIndices  = dae_mesh_needs_fix_indices(mesh);

  if (!needsTriangulate && !needsNormals && !needsFixIndices) {
    if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
      ak_bbox_mesh(mesh);
    return AK_OK;
  }

  tBegin = profile ? dae_mesh_profile_now_ms() : 0.0;
  ak_meshBeginEdit(mesh);
  tBegin = profile ? dae_mesh_profile_now_ms() - tBegin : 0.0;

  edith                 = mesh->edith;
  edith->skipFixIndices = true; /* to do it once per mesh */

  tTriangulate = profile ? dae_mesh_profile_now_ms() : 0.0;
  if (needsTriangulate)
    ak_meshTriangulate(mesh);
  tTriangulate = profile ? dae_mesh_profile_now_ms() - tTriangulate : 0.0;

  tNormals = profile ? dae_mesh_profile_now_ms() : 0.0;
  if (needsNormals)
    ak_meshGenNormals(mesh);
  tNormals = profile ? dae_mesh_profile_now_ms() - tNormals : 0.0;

  edith->skipFixIndices = false;
  tFixIndices = profile ? dae_mesh_profile_now_ms() : 0.0;
  if (needsFixIndices)
    ak_meshFixIndicesDefault(mesh);
  tFixIndices = profile ? dae_mesh_profile_now_ms() - tFixIndices : 0.0;

  tEnd = profile ? dae_mesh_profile_now_ms() : 0.0;
  ak_meshEndEdit(mesh);
  tEnd = profile ? dae_mesh_profile_now_ms() - tEnd : 0.0;

  tBBox = profile ? dae_mesh_profile_now_ms() : 0.0;
  if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
    ak_bbox_mesh(mesh);
  tBBox = profile ? dae_mesh_profile_now_ms() - tBBox : 0.0;

  if (profile) {
    t1 = dae_mesh_profile_now_ms() - t0;
    if (t1 > 0.25) {
      fprintf(stderr,
              "[AssetKit DAE mesh] total=%.3fms begin=%.3fms tri=%.3fms "
              "norm=%.3fms fixidx=%.3fms end=%.3fms bbox=%.3fms prim=%u\n",
              t1,
              tBegin,
              tTriangulate,
              tNormals,
              tFixIndices,
              tEnd,
              tBBox,
              mesh->primitiveCount);
    }
  }
  
  return AK_OK;
}
