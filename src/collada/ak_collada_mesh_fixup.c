/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_mesh_fixup.h"
#include "../ak_memory_common.h"
#include "../mesh/ak_mesh_index.h"

AkResult _assetkit_hide
ak_dae_mesh_fixup(AkMesh * mesh) {
  AkHeap *heap;
  AkDoc  *doc;

  heap = ak_heap_getheap(mesh->vertices);
  doc  = ak_heap_data(heap);

  /* first fixup coord system because verts will be duplicated,
     reduce extra process */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
      && (void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
    ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));

  ak_mesh_fix_indices(heap, mesh);

  if (ak_opt_get(AK_OPT_COMPUTE_BBOX))
    ak_bbox_mesh(mesh);

  if (ak_opt_get(AK_OPT_TRIANGULATE))
    ak_meshTriangulate(mesh);

  if (ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED))
    if (ak_meshNeedsNormals(mesh))
      ak_meshGenNormals(mesh);

  return AK_OK;
}
