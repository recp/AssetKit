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

#ifndef io_common_util_h
#define io_common_util_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"
#include "../../data.h"

#include <string.h>
#include <stdlib.h>

_assetkit_hide
AkMesh*
ak_allocMesh(AkHeap      * __restrict heap,
             AkLibrary   * __restrict memp,
             AkGeometry ** __restrict geomLink);

_assetkit_hide
AkInput*
io_addInput(AkHeap          * __restrict heap,
            AkDataContext   * __restrict dctx,
            AkMeshPrimitive * __restrict prim,
            AkInputSemantic              sem,
            const char      * __restrict semRaw,
            AkComponentSize              compSize,
            AkTypeId                     type,
            uint32_t                     offset);

#endif /* io_common_util_h */
