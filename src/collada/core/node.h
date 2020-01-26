/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_node_h_
#define __libassetkit__dae_node_h_

#include "../common.h"

AkResult _assetkit_hide
dae_node(AkXmlState    * __restrict xst,
         void          * __restrict memParent,
         AkVisualScene * __restrict scene,
         AkNode       ** __restrict dest);

#endif /* __libassetkit__dae_node_h_ */
