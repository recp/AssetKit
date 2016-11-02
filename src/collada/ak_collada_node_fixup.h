/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_collada_node_fixup_h
#define ak_collada_node_fixup_h

#include "ak_collada_common.h"

void _assetkit_hide
ak_dae_nodeFixupCamera(AkHeap * __restrict heap,
                       AkNode * __restrict node);

void _assetkit_hide
ak_dae_nodeFixup(AkHeap * __restrict heap,
                 AkDoc  * __restrict doc,
                 AkNode * __restrict node);

#endif /* ak_collada_node_fixup_h */
