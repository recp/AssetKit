/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_src_sid_h
#define ak_src_sid_h

#include "ak_common.h"

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
ak_sid_init();

void _assetkit_hide
ak_sid_deinit();

void
ak_sidInitConstr();

AkSidConstr*
ak_sidConstraintsOf(AkTypeId typeId);

void
ak_sidConstraintTo(AkTypeId         typeId,
                   AkSidConstrItem *constrs,
                   AkEnum           method);

#endif /* ak_src_sid_h */
