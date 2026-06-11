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

#ifndef assetkit_obj_exp_mesh_h
#define assetkit_obj_exp_mesh_h

#include "common.h"

AK_HIDE
bool
wobj_write_mesh_instance(WOBJExpState      * __restrict st,
                         AkNode            * __restrict node,
                         AkInstanceGeometry * __restrict inst,
                         AkGeometry        * __restrict geom,
                         mat4                            world);

#endif /* assetkit_obj_exp_mesh_h */
