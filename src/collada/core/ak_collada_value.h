/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_value__h_
#define __libassetkit__ak_collada_value__h_

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_value(AkDaeState * __restrict daestate,
             void * __restrict memParent,
             void ** __restrict dest,
             AkValueType * __restrict val_type);

AkEnum _assetkit_hide
ak_dae_valueType(const char * typeName);

#endif /* __libassetkit__ak_collada_value__h_ */
