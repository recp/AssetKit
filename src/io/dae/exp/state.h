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

#ifndef assetkit_dae_exp_state_h
#define assetkit_dae_exp_state_h

#include "common.h"

#include <stdio.h>

AK_HIDE
bool
dae_state_init(DAEExpState          * __restrict st,
               AkDoc                * __restrict doc,
               FILE                 * __restrict file,
               const char           * __restrict filepath,
               AkDaeExportIndexMode              indexMode,
               AkDaeExportVersion                versionMode);

AK_HIDE
void
dae_state_destroy(DAEExpState * __restrict st);

#endif /* assetkit_dae_exp_state_h */
