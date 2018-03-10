/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_color_h_
#define __libassetkit__dae_color_h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_color(AkXmlState * __restrict xst,
             void       * __restrict memParent,
             bool                    read_sid,
             bool                    stack,
             AkColor    * __restrict dest);

#endif /* __libassetkit__dae_color_h_ */
