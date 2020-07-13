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

#include "postscript.h"
#include "../../mesh/index.h"

void _assetkit_hide
stl_postscript(STLState * __restrict sst) {
  AkDoc      *doc;
  AkLibrary  *geomLib;
  AkGeometry *geom;

  doc = sst->doc;

  geomLib = doc->lib.geometries;
  while (geomLib) {
    geom = (void *)geomLib->chld;
    while (geom) {
      AkObject *primitive;

      primitive = geom->gdata;
      switch ((AkGeometryType)primitive->type) {
        case AK_GEOMETRY_MESH: {
          AkMesh           *mesh;
          AkMeshEditHelper *edith;

          mesh = ak_objGet(primitive);
          
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
          
        }
        default:
          break;
      }
      geom = (void *)geom->base.next;
    }

    geomLib = geomLib->next;
  }
  
}
