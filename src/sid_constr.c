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

#include "common.h"
#include "sid.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern AkHeap ak__sidconst_heap;

#define sidconst_heap &ak__sidconst_heap

AkTypeId ak__sidConstrChldParam[] = {
  AKT_NEWPARAM,
  AKT_PARAM,
  AKT_SETPARAM
};

AkSidConstrItem ak__sidConstrEffect = {
    .constr     = AKT_TECHNIQUE_FX,
    .chldCound  = 3,
    .constrChld = ak__sidConstrChldParam,
  .next = &(AkSidConstrItem){
    .constr     = AKT_PROFILE,
    .chldCound  = 3,
    .constrChld = ak__sidConstrChldParam,
  .next = &(AkSidConstrItem){
    .constr     = AKT_EFFECT,
    .chldCound  = 3,
    .constrChld = ak__sidConstrChldParam,
    }
  }
};

AkSidConstr*
ak_sidConstraintsOf(AkTypeId typeId) {
  void     *found;
  AkResult  ret;

  ret = ak_heap_getMemById(sidconst_heap,
                           &typeId,
                           &found);
  if (ret == AK_OK)
    return found;

  return NULL;
}

void
ak_sidConstraintTo(AkTypeId         typeId,
                   AkSidConstrItem *constrs,
                   AkEnum           method) {
  AkHeap      *heap;
  AkSidConstr *constr;
  void        *found;
  AkResult     ret;

  heap = sidconst_heap;
  ret  = ak_heap_getMemById(heap,
                            &typeId,
                            &found);
  if (ret == AK_OK)
    ak_free(found);

  if (!constrs)
    return;

  constr = ak_heap_alloc(heap, NULL, sizeof(*constr));
  constr->method = method;
  constr->item   = constrs;
  constr->typeId = typeId;

  ak_heap_setId(heap,
                ak__alignof(constr),
                &constr[0]);
}

void
ak_sidInitConstr() {
  ak_sidConstraintTo(AKT_TEXTURE_NAME, &ak__sidConstrEffect, 0);
  ak_sidConstraintTo(AKT_TEXCOORD,     &ak__sidConstrEffect, 0);
  ak_sidConstraintTo(AKT_TEXTURE,      &ak__sidConstrEffect, 0);
}
