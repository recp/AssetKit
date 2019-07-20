/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_asset_h
#define gltf_asset_h

#include "../gltf_common.h"

AkResult _assetkit_hide
gltf_asset(AkGLTFState  * __restrict gst,
           void         * __restrict memParent,
           const json_t * __restrict json,
           AkAssetInf   * __restrict dest);

#endif /* gltf_asset_h */
