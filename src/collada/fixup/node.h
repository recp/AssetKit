/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_node_fixup_h
#define dae_node_fixup_h

#include "dae_common.h"

void _assetkit_hide
dae_nodeFixupFixedCoord(AkHeap * __restrict heap,
                        AkNode * __restrict node);

void _assetkit_hide
dae_nodeFixup(AkHeap * __restrict heap,
              AkNode*  __restrict node);

#endif /* dae_node_fixup_h */
