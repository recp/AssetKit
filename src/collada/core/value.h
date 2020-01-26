/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_value__h_
#define __libassetkit__dae_value__h_

#include "../common.h"

AkResult _assetkit_hide
dae_value(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          AkValue   ** __restrict dest);

void _assetkit_hide
dae_dtype(const char *typeName, AkTypeDesc *type);

#endif /* __libassetkit__dae_value__h_ */
