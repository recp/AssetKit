/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_enums_h
#define gltf_enums_h

#include "../gltf_common.h"

AkEnum _assetkit_hide
gltf_enumInputSemantic(const char *name);

AkEnum _assetkit_hide
gltf_componentType(int type);

int _assetkit_hide
gltf_componentLen(int type) ;

AkComponentSize _assetkit_hide
gltf_type(const char *name, size_t len);

AkEnum _assetkit_hide
gltf_minFilter(int type);

AkEnum _assetkit_hide
gltf_magFilter(int type);

AkEnum _assetkit_hide
gltf_wrapMode(int type);

AkOpaque _assetkit_hide
gltf_alphaMode(const char *name);

AkInterpolationType _assetkit_hide
gltf_interp(json_t * __restrict json);

#endif /* gltf_enums_h */
