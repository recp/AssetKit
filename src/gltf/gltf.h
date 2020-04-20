/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_h
#define gltf_h

#include "common.h"

AkResult _assetkit_hide
gltf_gltf(AkDoc     ** __restrict dest,
          const char * __restrict filepath);

AkResult _assetkit_hide
gltf_glb(AkDoc     ** __restrict dest,
         const char * __restrict filepath);

#endif /* gltf_h */
