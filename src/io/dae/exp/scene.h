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

#ifndef assetkit_dae_exp_scene_h
#define assetkit_dae_exp_scene_h

#include "common.h"

AK_HIDE
bool
dae_doc_has_unsupported_features(AkDoc * __restrict doc);

AK_HIDE
bool
dae_prepare_maps(DAEExpState * __restrict st);

AK_HIDE
void
dae_write_visual_scene(DAEExpState * __restrict st,
                       AkScene     * __restrict scene,
                       uint32_t                 sceneIdx);

AK_HIDE
void
dae_write_library_nodes(DAEExpState * __restrict st);

AK_HIDE
uint32_t
dae_active_scene_index(AkDoc * __restrict doc);

#endif /* assetkit_dae_exp_scene_h */
