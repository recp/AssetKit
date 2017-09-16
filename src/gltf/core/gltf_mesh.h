/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_mesh_h
#define gltf_mesh_h

#include "../gltf_common.h"

void _assetkit_hide
gltf_meshes(AkGLTFState * __restrict gst);

AkMeshPrimitive* _assetkit_hide
gltf_allocPrim(AkHeap * __restrict heap,
               void   * __restrict memParent,
               int                 mode);

#endif /* gltf_mesh_h */
