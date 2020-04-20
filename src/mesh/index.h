/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_mesh_index_h
#define ak_mesh_index_h

#include "../common.h"

_assetkit_hide
AkResult
ak_meshFixIndices(AkMesh *mesh);

_assetkit_hide
AkResult
ak_primFixIndices(AkMesh          *mesh,
                  AkMeshPrimitive *prim);

_assetkit_hide
AkResult
ak_meshFixIndicesDefault(AkMesh *mesh);

#endif /* ak_mesh_index_h */
