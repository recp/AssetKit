/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
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

void _assetkit_hide
ak_sid_init(void);

void _assetkit_hide
ak_sid_deinit(void);

void
ak_sidInitConstr(void);

AkSidConstr*
ak_sidConstraintsOf(AkTypeId typeId);

void
ak_sidConstraintTo(AkTypeId         typeId,
                   AkSidConstrItem *constrs,
                   AkEnum           method);

_assetkit_hide
AkHeapNode*
ak_sid_profile(AkContext  * __restrict ctx,
               AkHeapNode * __restrict parent,
               AkHeapNode * __restrict after);

_assetkit_hide
AkHeapNode*
ak_sid_technique(AkContext  * __restrict ctx,
                 AkHeapNode * __restrict chld);

_assetkit_hide
AkHeapNode*
ak_sid_chldh(AkContext  * __restrict ctx,
             AkHeapNode * __restrict parent,
             AkHeapNode * __restrict after);

_assetkit_hide
ptrdiff_t
ak_sidElement(AkContext  * __restrict ctx,
              const char * __restrict target,
              void      ** __restrict idnode,
              bool       * __restrict isdot);

#endif /* ak_src_sid_h */
