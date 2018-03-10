/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_light__h_
#define __libassetkit__dae_light__h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_light(AkXmlState * __restrict xst,
             void       * __restrict memParent,
             void      ** __restrict  dest);

AkResult _assetkit_hide
ak_dae_light_tcommon(AkXmlState   * __restrict xst,
                     void         * __restrict memParent,
                     AkLightBase ** __restrict dest);

#endif /* __libassetkit__dae_light__h_ */
