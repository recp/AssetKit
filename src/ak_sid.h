/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_src_sid_h
#define ak_src_sid_h

#include "ak_common.h"

void _assetkit_hide
ak_sid_init();

void _assetkit_hide
ak_sid_deinit();

AkTypeId*
ak_sidConstraintsOf(AkTypeId typeId);

void
ak_sidConstraintTo(AkTypeId typeId,
                   AkTypeId typeIds[],
                   uint32_t count);

#endif /* ak_src_sid_h */
