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
#include <string.h>
#include <time.h>

typedef struct DaeMeshFixupProfile {
  double   total;
  double   topofix;
  double   scan;
  double   begin;
  double   triangulate;
  double   normals;
  double   fixIndices;
  double   end;
  double   bbox;
  uint32_t meshCount;
  uint32_t editCount;
  uint32_t skipCount;
  uint32_t primitiveCount;
} DaeMeshFixupProfile;

static DaeMeshFixupProfile dae_mesh_prof;

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

    if (!*needsFixIndices && prim->indices && prim->indexStride > 1)
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
void
dae_mesh_profile_reset(void) {
  memset(&dae_mesh_prof, 0, sizeof(dae_mesh_prof));
  ak_index_profile_reset();
}

AK_HIDE
void
dae_mesh_profile_report(void) {
  fprintf(stderr,
          "[AssetKit DAE mesh total] total=%.3fms mesh=%u edit=%u skip=%u "
          "prim=%u topofix=%.3fms scan=%.3fms begin=%.3fms tri=%.3fms "
          "norm=%.3fms fixidx=%.3fms end=%.3fms bbox=%.3fms\n",
          dae_mesh_prof.total,
          dae_mesh_prof.meshCount,
          dae_mesh_prof.editCount,
          dae_mesh_prof.skipCount,
          dae_mesh_prof.primitiveCount,
          dae_mesh_prof.topofix,
          dae_mesh_prof.scan,
          dae_mesh_prof.begin,
          dae_mesh_prof.triangulate,
          dae_mesh_prof.normals,
          dae_mesh_prof.fixIndices,
          dae_mesh_prof.end,
          dae_mesh_prof.bbox);
  ak_index_profile_report();
}

AK_HIDE
AkResult
dae_mesh_fixup(AkMesh * mesh) {
  AkMeshEditHelper *edith;
  AkHeap           *heap;
  AkDoc            *doc;
  double            t0, t1, tTopofix, tScan, tBegin, tTriangulate, tNormals,
                    tFixIndices, tEnd, tBBox;
  bool              profile, needsTriangulate, needsNormals, needsFixIndices;

  heap = ak_heap_getheap(mesh->geom);
  doc  = ak_heap_data(heap);
  profile = dae_mesh_profile_enabled();
  t0 = profile ? dae_mesh_profile_now_ms() : 0.0;
  tTopofix = tScan = tBegin = tTriangulate = tNormals = tFixIndices = 0.0;
  tEnd = tBBox = 0.0;

  if (profile) {
    dae_mesh_prof.meshCount++;
    dae_mesh_prof.primitiveCount += mesh->primitiveCount;
  }

  tTopofix = profile ? dae_mesh_profile_now_ms() : 0.0;
  topofix(mesh);
  tTopofix = profile ? dae_mesh_profile_now_ms() - tTopofix : 0.0;

  /* first fixup coord system because verts will be duplicated,
     reduce extra process */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
      && (void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
    ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));

  if (!mesh->primitive)
    return AK_OK;

  tScan = profile ? dae_mesh_profile_now_ms() : 0.0;
  dae_mesh_needs_edit(mesh,
                      ak_opt_get(AK_OPT_TRIANGULATE),
                      ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED),
                      &needsTriangulate,
                      &needsNormals,
                      &needsFixIndices);
  tScan = profile ? dae_mesh_profile_now_ms() - tScan : 0.0;

  if (!needsTriangulate && !needsNormals && !needsFixIndices) {
    tBBox = profile ? dae_mesh_profile_now_ms() : 0.0;
    if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
      ak_bbox_mesh(mesh);
    tBBox = profile ? dae_mesh_profile_now_ms() - tBBox : 0.0;
    if (profile) {
      t1 = dae_mesh_profile_now_ms() - t0;
      dae_mesh_prof.total   += t1;
      dae_mesh_prof.topofix += tTopofix;
      dae_mesh_prof.scan    += tScan;
      dae_mesh_prof.bbox    += tBBox;
      dae_mesh_prof.skipCount++;
    }
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
    ak_meshFixIndicesDefaultRetainDuplicators(mesh,
                                              doc->lib.controllers != NULL
                                              || mesh->skins != NULL);
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
    dae_mesh_prof.total      += t1;
    dae_mesh_prof.topofix    += tTopofix;
    dae_mesh_prof.scan       += tScan;
    dae_mesh_prof.begin      += tBegin;
    dae_mesh_prof.triangulate += tTriangulate;
    dae_mesh_prof.normals    += tNormals;
    dae_mesh_prof.fixIndices += tFixIndices;
    dae_mesh_prof.end        += tEnd;
    dae_mesh_prof.bbox       += tBBox;
    dae_mesh_prof.editCount++;
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
