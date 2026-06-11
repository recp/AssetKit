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

#ifndef assetkit_dae_exp_brep_h
#define assetkit_dae_exp_brep_h

#include "common.h"

AK_HIDE
bool
dae_spline_supported(AkSpline * __restrict spline);

AK_HIDE
bool
dae_brep_supported(AkBoundryRep * __restrict brep);

AK_HIDE
bool
dae_write_spline_geometry(DAEExpState * __restrict st,
                          AkGeometry  * __restrict geom,
                          uint32_t                  geomIdx);

AK_HIDE
bool
dae_write_brep_geometry(DAEExpState * __restrict st,
                        AkGeometry  * __restrict geom,
                        uint32_t                  geomIdx);

#endif /* assetkit_dae_exp_brep_h */
