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

#ifndef assetkit_dae_exp_source_h
#define assetkit_dae_exp_source_h

#include "common.h"

AK_HIDE
const char*
dae_semantic_name(AkInput * __restrict input);

AK_HIDE
DAEExpName
dae_param_exp_name(uint32_t idx);

AK_HIDE
DAEExpName
dae_input_param_exp_name(AkInput * __restrict input, uint32_t idx);

AK_HIDE
bool
dae_accessor_float_direct(AkAccessor * __restrict acc);

AK_HIDE
const float*
dae_accessor_float_row(AkAccessor * __restrict acc, uint32_t index);

AK_HIDE
bool
dae_write_source(DAEExpState  * __restrict st,
                 AkInput      * __restrict input,
                 uint32_t                  geomIdx,
                 uint32_t                  primIdx,
                 uint32_t                  inputIdx);

#endif /* assetkit_dae_exp_source_h */
