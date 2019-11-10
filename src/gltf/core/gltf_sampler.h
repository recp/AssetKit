/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_sampler_h
#define gltf_sampler_h

#include "../gltf_common.h"

AkSampler* _assetkit_hide
gltf_newsampler(AkGLTFState * __restrict gst,
                AkEffect    * __restrict effect);

void _assetkit_hide
gltf_samplers(json_t * __restrict jsampler,
              void   * __restrict userdata);

#endif /* gltf_sampler_h */
