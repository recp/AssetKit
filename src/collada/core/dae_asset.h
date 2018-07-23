/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_asset__h_
#define __libassetkit__dae_asset__h_

#include "../dae_common.h"

AkResult _assetkit_hide
dae_assetInf(AkXmlState * __restrict xst,
             void       * __restrict memParent,
             AkAssetInf * __restrict dest);

#endif /* __libassetkit__dae_asset__h_ */
