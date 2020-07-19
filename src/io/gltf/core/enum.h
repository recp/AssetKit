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

AkEnum AK_HIDE
gltf_enumInputSemantic(const char *name);

AkEnum AK_HIDE
gltf_componentType(int type);

int AK_HIDE
gltf_componentLen(int type) ;

AkComponentSize AK_HIDE
gltf_type(const json_t * __restrict json);

AkEnum AK_HIDE
gltf_minFilter(int type);

AkEnum AK_HIDE
gltf_magFilter(int type);

AkEnum AK_HIDE
gltf_wrapMode(int type);

AkOpaque AK_HIDE
gltf_alphaMode(const json_t * __restrict json);

AkInterpolationType AK_HIDE
gltf_interp(const json_t * __restrict json);

#endif /* gltf_enums_h */
