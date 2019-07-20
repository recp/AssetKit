/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_buffer_h
#define gltf_buffer_h

#include "../gltf_common.h"

AkResult _assetkit_hide
gltf_buffers(AkGLTFState  * __restrict gst,
             const json_t * __restrict json);

AkResult _assetkit_hide
gltf_bufferViews(AkGLTFState  * __restrict gst,
                 const json_t * __restrict json);

#endif /* gltf_buffer_h */
