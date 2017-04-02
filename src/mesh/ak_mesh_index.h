/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_mesh_index_h
#define ak_mesh_index_h

#include "../ak_common.h"

_assetkit_hide
AkResult
ak_mesh_fix_indices(AkHeap *heap, AkMesh *mesh);

_assetkit_hide
AkResult
ak_mesh_fix_pos(AkHeap   *heap,
                AkMesh   *mesh,
                AkSource *oldSrc, /* caller alreay has position source */
                uint32_t  newstride);

_assetkit_hide
AkResult
ak_mesh_fix_idx_df(AkHeap *heap, AkMesh *mesh);

#endif /* ak_mesh_index_h */
