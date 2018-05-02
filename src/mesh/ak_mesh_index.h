/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_mesh_index_h
#define ak_mesh_index_h

#include "../ak_common.h"

_assetkit_hide
AkResult
ak_meshFixIndices(AkHeap *heap, AkMesh *mesh);

_assetkit_hide
AkResult
ak_primFixIndices(AkHeap          *heap,
                  AkMesh          *mesh,
                  AkMeshPrimitive *prim);

_assetkit_hide
AkResult
ak_meshFixIndicesDefault(AkHeap *heap, AkMesh *mesh);

#endif /* ak_mesh_index_h */
