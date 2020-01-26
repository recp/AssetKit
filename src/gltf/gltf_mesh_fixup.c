/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_mesh_fixup.h"
#include "../mesh/mesh_index.h"

void _assetkit_hide
gltf_mesh_fixup(AkGLTFState * __restrict gst) {
  AkDoc      *doc;
  AkLibItem  *geomLib;
  AkGeometry *geom;

  doc = gst->doc;

  geomLib = doc->lib.geometries;
  while (geomLib) {
    geom = geomLib->chld;
    while (geom) {
      AkObject *primitive;

      primitive = geom->gdata;
      switch ((AkGeometryType)primitive->type) {
        case AK_GEOMETRY_MESH: {
          AkMesh *mesh;
          mesh = ak_objGet(primitive);

          /* first fixup coord system because verts will be duplicated,
             reduce extra process */
          if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
              && (void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
            ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));

          if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
            ak_bbox_mesh(mesh);

          if (ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED))
            if (ak_meshNeedsNormals(mesh))
              ak_meshGenNormals(mesh);
        }
        default:
          break;
      }
      geom = geom->next;
    }

    geomLib = geomLib->next;
  }
 }
