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

#ifndef io_common_primitive_h
#define io_common_primitive_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"

#include <stdbool.h>
#include <stdint.h>

AK_HIDE
uint32_t
io_primitive_vertex_count(AkMeshPrimitive * __restrict prim);

AK_HIDE
AkUInt
io_primitive_input_index(AkMeshPrimitive * __restrict prim,
                         AkInput         * __restrict input,
                         uint32_t                     vertexIndex);

AK_HIDE
bool
io_accessor_float_direct(AkAccessor * __restrict acc);

AK_HIDE
const float*
io_accessor_float_row(AkAccessor * __restrict acc, uint32_t index);

#endif /* io_common_primitive_h */
