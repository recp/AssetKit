/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_node_h
#define dae_node_h

#include "../common.h"

_assetkit_hide
void*
dae_node2(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

_assetkit_hide
AkNode*
dae_node(DAEState      * __restrict dst,
         xml_t         * __restrict xml,
         void          * __restrict memp,
         AkVisualScene * __restrict scene);

#endif /* dae_node_h */
