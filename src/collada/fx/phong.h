/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_blinn_phong__h_
#define __libassetkit__dae_blinn_phong__h_

#include "../../../include/ak/assetkit.h"
#include "../dae_common.h"

AkResult _assetkit_hide
dae_phong(AkXmlState           * __restrict xst,
          void                 * __restrict memParent,
          const char           * elm,
          AkTechniqueFxCommon ** __restrict dest);

#endif /* __libassetkit__dae_blinn_phong__h_ */
