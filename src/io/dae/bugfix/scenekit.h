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

#ifndef dae_bugfix_scenekit_h
#define dae_bugfix_scenekit_h

#include "../common.h"

AK_HIDE
bool
dae_scenekit_authored(AkDoc * __restrict doc);

AK_HIDE
void
dae_scenekit_normalize_colors(DAEState * __restrict dst);

AK_HIDE
void
dae_scenekit_normalize_animation_colors(DAEState * __restrict dst);

AK_HIDE
void
dae_bugfix_scenekit_backfaces(DAEState * __restrict dst);

AK_HIDE
void
dae_bugfix_scenekit_material_surfaces(DAEState * __restrict dst);

#endif /* dae_bugfix_scenekit_h */
