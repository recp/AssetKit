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

#ifndef assetkit_dae_exp_mesh_h
#define assetkit_dae_exp_mesh_h

#include "common.h"
#include "../../common/primitive.h"

AK_HIDE
bool
dae_prepare_geometry_index_mode(DAEExpState * __restrict st,
                                AkGeometry  * __restrict geom);

AK_HIDE
bool
dae_prepare_extra_geometry(DAEExpState * __restrict st,
                           AkGeometry  * __restrict geom);

AK_HIDE
bool
dae_geometry_supported(AkGeometry * __restrict geom);

AK_HIDE
bool
dae_primitive_supported(AkMeshPrimitive * __restrict prim);

AK_HIDE
bool
dae_primitive_tag(AkMeshPrimitive * __restrict prim,
                  DAEExpName      * __restrict tag);

AK_HIDE
uint32_t
dae_primitive_count(AkMeshPrimitive * __restrict prim);

AK_HIDE
AkGeometry*
dae_morph_target_geometry(AkMorphTarget * __restrict target);

AK_HIDE
bool
dae_instance_morph_supported(AkInstanceGeometry * __restrict inst);

AK_HIDE
void
dae_mark_morph_vertex_geometry(DAEExpState * __restrict st,
                               AkGeometry  * __restrict geom);

AK_HIDE
bool
dae_prepare_morph_target_geometries(DAEExpState * __restrict st,
                                    AkMorph     * __restrict morph);

AK_HIDE
bool
dae_prepare_morph(DAEExpState * __restrict st,
                  AkMorph     * __restrict morph);

AK_HIDE
void
dae_w_morph_target_geom_id(DAEExpWriter * __restrict w,
                           uint32_t                  morphIdx,
                           uint32_t                  targetIdx);

AK_HIDE
bool
dae_write_morphable_target_geometries(DAEExpState * __restrict st,
                                      AkMorph     * __restrict morph,
                                      AkGeometry  * __restrict baseGeom,
                                      uint32_t                 morphIdx);

AK_HIDE
bool
dae_write_geometry(DAEExpState * __restrict st,
                   AkGeometry  * __restrict geom,
                   uint32_t                 geomIdx);

#endif /* assetkit_dae_exp_mesh_h */
