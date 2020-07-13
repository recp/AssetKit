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

#ifndef wobj_util_h
#define wobj_util_h

#include "common.h"
#include "../common/util.h"

_assetkit_hide
void
wobj_fixIndices(AkMeshPrimitive * __restrict prim);

_assetkit_hide
AkInput*
wobj_addInput(WOState         * __restrict wst,
              AkDataContext   * __restrict dctx,
              AkMeshPrimitive * __restrict prim,
              AkInputSemantic              sem,
              const char      * __restrict semRaw,
              AkComponentSize              compSize,
              AkTypeId                     type,
              uint32_t                     offset);

_assetkit_hide
void
wobj_joinIndices(WOState * __restrict wst, AkMeshPrimitive * __restrict prim);

#endif /* wobj_util_h */
