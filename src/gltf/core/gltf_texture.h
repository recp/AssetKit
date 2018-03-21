/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_texture_h
#define gltf_texture_h

#include "../gltf_common.h"

AkTextureRef* _assetkit_hide
gltf_texref(AkGLTFState * __restrict gst,
            AkEffect    * __restrict effect,
            void        * __restrict parent,
            json_t      * __restrict jtexinfo);

AkSampler* _assetkit_hide
gltf_texture(AkGLTFState * __restrict gst,
             AkEffect    * __restrict effect,
             int32_t                  index);

#endif /* gltf_texture_h */
