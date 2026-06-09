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

#ifndef assetkit_gltf_exp_plan_h
#define assetkit_gltf_exp_plan_h

#include "common.h"

void
gltf_plan(GLTFExpState * __restrict st);

bool
gltf_input_supported(AkInput * __restrict input);

bool
gltf_normal_input_valid(AkInput * __restrict input);

bool
gltf_input_count_valid(AkMeshPrimitive * __restrict prim,
                       AkInput         * __restrict input,
                       AkInput         * __restrict posInput);

uint32_t
gltf_input_source_set(AkInput * __restrict input);

uint32_t
gltf_input_export_set(AkMeshPrimitive * __restrict prim,
                      AkInput         * __restrict input);

int32_t
gltf_texcoord_export_set(AkMeshPrimitive * __restrict prim,
                         int32_t                       sourceSet);

bool
gltf_texcoord_source_set_valid(AkMeshPrimitive * __restrict prim,
                               int32_t                       sourceSet);

AkInput*
gltf_primitive_position_input(AkMeshPrimitive * __restrict prim);

bool
gltf_primitive_mode(AkMeshPrimitive * __restrict prim, GLTFExpIndex *mode);

bool
gltf_ptrs_add(GLTFExpPtrTable * __restrict table, void * __restrict ptr);

GLTFExpIndex
gltf_ptrs_index(GLTFExpPtrTable * __restrict table, void * __restrict ptr);

GLTFExpIndex
gltf_node_index(GLTFExpState * __restrict st,
                AkNode       * __restrict node);

GLTFExpIndex
gltf_skin_index(GLTFExpState * __restrict st,
                AkInstanceSkin * __restrict skinner);

GLTFExpIndex
gltf_accessor_index(GLTFExpAccessorTable * __restrict table,
                    AkAccessor           * __restrict accessor);

GLTFExpIndex
gltf_position_accessor_index(GLTFExpState    * __restrict st,
                             AkMeshPrimitive * __restrict prim);

GLTFExpIndex
gltf_baked_accessor_index(GLTFExpState     * __restrict st,
                          AkNode           * __restrict node,
                          AkMeshPrimitive  * __restrict prim,
                          AkInputSemantic                 semantic);

GLTFExpIndex
gltf_prim_index_accessor_index(GLTFExpAccessorTable * __restrict table,
                               AkMeshPrimitive      * __restrict prim);

#endif /* assetkit_gltf_exp_plan_h */
