/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_texture_h
#define gltf_texture_h

#include "../common.h"

AkTextureRef* _assetkit_hide
gltf_texref(AkGLTFState * __restrict gst,
            void        * __restrict parent,
            json_t      * __restrict jtexinfo);

void _assetkit_hide
gltf_textures(json_t * __restrict jtex,
              void   * __restrict userdata);

#endif /* gltf_texture_h */
