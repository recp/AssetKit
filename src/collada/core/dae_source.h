/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_source_h_
#define __libassetkit__dae_source_h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_source(AkXmlState * __restrict xst,
              void       * __restrict memParent,
              AkSource  ** __restrict dest);

#endif /* __libassetkit__dae_source_h_ */
