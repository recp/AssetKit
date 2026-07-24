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

#ifndef dae_index_parse_h
#define dae_index_parse_h

#include "../common.h"

AK_HIDE
bool
dae_defer_index_array(DAEState       * __restrict dst,
                      const xml_t    * __restrict source,
                      AkMeshPrimitive * __restrict primitive,
                      void           * __restrict parent,
                      unsigned long               count);

AK_HIDE
void
dae_parse_index_arrays(DAEState * __restrict dst);

#endif /* dae_index_parse_h */
