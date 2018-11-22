/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_skin_h_
#define __libassetkit__dae_skin_h_

#include "../dae_common.h"

AkResult _assetkit_hide
dae_skin(AkXmlState * __restrict xst,
         void       * __restrict memParent,
         bool                    asObject,
         AkSkin    ** __restrict dest);

#endif /* __libassetkit__dae_skin_h_ */
