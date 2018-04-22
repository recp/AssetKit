/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_mesh_util_h
#define ak_mesh_util_h

#include "../ak_common.h"

size_t
ak_mesh_src_usg(AkHeap *heap,
                AkMesh *mesh,
                AkSource *src);

AkSource *
ak_mesh_src(AkHeap   *heap,
            AkMesh   *mesh,
            AkSource *src,
            uint32_t  max);

AkSource *
ak_mesh_src_for(AkHeap         *heap,
                AkMesh         *mesh,
                AkInputSemantic semantic);

AkSource*
ak_mesh_src_for_ext(AkHeap         *heap,
                    AkMesh         *mesh,
                    char           *srcid,
                    AkInputSemantic semantic,
                    size_t          count);

AkSource *
ak_mesh_pos_src(AkMesh *mesh);

AkSource*
ak_mesh_positions(AkMesh * __restrict mesh);

uint32_t
ak_mesh_arr_stride(AkMesh *mesh, AkURL *arrayURL);

size_t
ak_mesh_intr_count(AkMesh *mesh);

void
ak_accessor_rebound(AkHeap     *heap,
                    AkAccessor *acc,
                    uint32_t    offset);

#endif /* ak_mesh_util_h */
