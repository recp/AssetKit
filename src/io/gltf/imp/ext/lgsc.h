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

#ifndef ak_gltf_ext_lgsc_h
#define ak_gltf_ext_lgsc_h

#include "../common.h"

#define AK_GLTF_LGSC_EXT "KHR_gaussian_splatting_compression_lgsc"

AK_HIDE
bool
gltf_lgsc_prepare(AkGLTFState *gst, const json_t *root);

AK_HIDE
bool
gltf_lgsc_primitive(AkGLTFState *gst, AkMeshPrimitive *prim, const json_t *extension);

#endif /* ak_gltf_ext_lgsc_h */
