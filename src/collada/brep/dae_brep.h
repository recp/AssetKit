/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_brep_h_
#define __libassetkit__dae_brep_h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_brep(AkXmlState * __restrict xst,
            void * __restrict memParent,
            bool asObject,
            AkBoundryRep ** __restrict dest);

#endif /* __libassetkit__dae_brep_h_ */
