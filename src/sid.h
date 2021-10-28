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

#ifndef ak_src_sid_h
#define ak_src_sid_h

#include "common.h"

typedef struct AkSidConstrItem {
  AkTypeId                constr;
  uint32_t                chldCound;
  AkTypeId               *constrChld;
  struct AkSidConstrItem *next;
} AkSidConstrItem;

typedef struct AkSidConstr {
  AkTypeId         typeId;
  AkEnum           method; /* 0: block-scope */
  AkSidConstrItem *item;
} AkSidConstr;

AK_HIDE void
ak_sid_init(void);

AK_HIDE void
ak_sid_deinit(void);

void
ak_sidInitConstr(void);

AkSidConstr*
ak_sidConstraintsOf(AkTypeId typeId);

void
ak_sidConstraintTo(AkTypeId         typeId,
                   AkSidConstrItem *constrs,
                   AkEnum           method);

AK_HIDE
AkHeapNode*
ak_sid_profile(AkContext  * __restrict ctx,
               AkHeapNode * __restrict parent,
               AkHeapNode * __restrict after);

AK_HIDE
AkHeapNode*
ak_sid_technique(AkContext  * __restrict ctx,
                 AkHeapNode * __restrict chld);

AK_HIDE
AkHeapNode*
ak_sid_chldh(AkContext  * __restrict ctx,
             AkHeapNode * __restrict parent,
             AkHeapNode * __restrict after);

AK_HIDE
ptrdiff_t
ak_sidElement(AkContext  * __restrict ctx,
              const char * __restrict target,
              void      ** __restrict idnode,
              bool       * __restrict isdot);

#endif /* ak_src_sid_h */
