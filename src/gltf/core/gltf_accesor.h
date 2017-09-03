/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_accesor_h
#define gltf_accesor_h

#include "../gltf_common.h"

AkAccessor* _assetkit_hide
gltf_accessor(AkGLTFState     * __restrict gst,
              AkMeshPrimitive * __restrict prim,
              json_t          * __restrict jacc);

#endif /* gltf_accesor_h */
