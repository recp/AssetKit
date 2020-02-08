/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_node_h_
#define __libassetkit__dae_node_h_

#include "../common.h"

AkNode* _assetkit_hide
dae_node(DAEState      * __restrict dst,
         xml_t         * __restrict xml,
         void          * __restrict memp,
         AkVisualScene * __restrict scene);

#endif /* __libassetkit__dae_node_h_ */
