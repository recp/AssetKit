/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef gltf_enums_h
#define gltf_enums_h

#include "../common.h"

AK_HIDE AkEnum
gltf_enumInputSemantic(const char *name);

AK_HIDE AkEnum
gltf_componentType(int type);

AK_HIDE int
gltf_componentLen(int type) ;

AK_HIDE AkComponentSize
gltf_type(const json_t * __restrict json);

AK_HIDE AkEnum
gltf_minFilter(int type);

AK_HIDE AkEnum
gltf_magFilter(int type);

AK_HIDE AkEnum
gltf_wrapMode(int type);

AK_HIDE AkOpaque
gltf_alphaMode(const json_t * __restrict json);

AK_HIDE AkInterpolationType
gltf_interp(const json_t * __restrict json);

#endif /* gltf_enums_h */
