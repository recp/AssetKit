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

#ifndef assetkit_dae_exp_material_h
#define assetkit_dae_exp_material_h

#include "common.h"

AK_HIDE
bool
dae_prepare_material_dependencies(DAEExpState * __restrict st,
                                  AkMaterial  * __restrict mat);

AK_HIDE
bool
dae_prepare_extra_material(DAEExpState * __restrict st,
                           AkMaterial  * __restrict mat);

AK_HIDE
bool
dae_prepare_instance_materials(DAEExpState        * __restrict st,
                               AkGeometry         * __restrict geom,
                               AkInstanceGeometry * __restrict inst);

AK_HIDE
void
dae_write_effect(DAEExpState * __restrict st,
                 AkMaterial  * __restrict mat,
                 uint32_t                 matIdx);

AK_HIDE
void
dae_write_material(DAEExpState * __restrict st,
                   AkMaterial  * __restrict mat,
                   uint32_t                 matIdx);

AK_HIDE
void
dae_write_instance_materials(DAEExpState        * __restrict st,
                             AkGeometry         * __restrict geom,
                             AkInstanceGeometry * __restrict inst);

#endif /* assetkit_dae_exp_material_h */
