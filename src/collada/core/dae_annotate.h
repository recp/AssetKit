/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_annotate__h_
#define __libassetkit__dae_annotate__h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_annotate(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkAnnotate ** __restrict dest);

#endif /* __libassetkit__dae_annotate__h_ */
