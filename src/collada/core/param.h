/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_param__h_
#define __libassetkit__dae_param__h_

#include "../common.h"

AkResult _assetkit_hide
dae_newparam(AkXmlState  * __restrict xst,
             void        * __restrict memParent,
             AkNewParam ** __restrict dest);

AkResult _assetkit_hide
dae_param(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          AkParam   ** __restrict dest);

#endif /* __libassetkit__dae_param__h_ */
