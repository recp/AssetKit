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

#ifndef gltf_imp_core_mesh_h
#define gltf_imp_core_mesh_h

#include "../common.h"

AK_HIDE
void
gltf_meshes(json_t * __restrict json,
            void   * __restrict userdata);

AK_HIDE
AkMeshPrimitive*
gltf_allocPrim(AkHeap * __restrict heap,
               void   * __restrict memParent,
               int                 mode);

AK_HIDE
uint32_t
gltf_polyCount(AkMeshPrimitive *prim, uint32_t mode);

#endif /* gltf_imp_core_mesh_h */
