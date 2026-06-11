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

#ifndef assetkit_dae_exp_controller_h
#define assetkit_dae_exp_controller_h

#include "common.h"

AK_HIDE
bool
dae_instance_skin_supported(AkInstanceGeometry * __restrict inst);

AK_HIDE
void
dae_w_skin_id(DAEExpWriter * __restrict w, uint32_t skinIdx);

AK_HIDE
void
dae_w_morph_id(DAEExpWriter * __restrict w, uint32_t morphIdx);

AK_HIDE
bool
dae_write_skin_controller(DAEExpState * __restrict st,
                          AkSkin      * __restrict skin,
                          uint32_t                 skinIdx);

AK_HIDE
bool
dae_write_morph_controller(DAEExpState * __restrict st,
                           AkMorph     * __restrict morph,
                           uint32_t                 morphIdx);

AK_HIDE
void
dae_write_library_controllers(DAEExpState * __restrict st);

AK_HIDE
void
dae_write_instance_controller(DAEExpState        * __restrict st,
                              AkInstanceGeometry * __restrict inst);

AK_HIDE
bool
dae_instance_controller_exportable(DAEExpState        * __restrict st,
                                   AkInstanceGeometry * __restrict inst);

#endif /* assetkit_dae_exp_controller_h */
