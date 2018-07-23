/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_render_h_
#define __libassetkit__dae_render_h_

#include "../dae_common.h"

AkResult _assetkit_hide
dae_render(AkXmlState * __restrict xst,
           void * __restrict memParent,
           AkRender ** __restrict dest);

#endif /* __libassetkit__dae_render_h_ */
