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

#ifndef assetkit_gltf_exp_bin_h
#define assetkit_gltf_exp_bin_h

#include "common.h"

#include <stdbool.h>
#include <stdio.h>

AkResult
gltf_prepare_bin(GLTFExpState * __restrict st);

bool
gltf_write_bin_payload(GLTFExpState * __restrict st,
                       FILE         * __restrict file);

AkResult
gltf_write_bin(GLTFExpState * __restrict st,
               const char   * __restrict filepath);

#endif /* assetkit_gltf_exp_bin_h */
