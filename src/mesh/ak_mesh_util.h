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
ak_mesh_pos_src(AkMesh *mesh);

uint32_t
ak_mesh_vert_stride(AkMesh *mesh);

uint32_t
ak_mesh_arr_stride(AkMesh *mesh, AkURL *arrayURL);

uint32_t
ak_mesh_prim_stride(AkMeshPrimitive *prim);

size_t
ak_mesh_intr_count(AkMesh *mesh);

int
ak_mesh_vertex_off(AkMeshPrimitive *prim);

void
ak_accessor_rebound(AkHeap     *heap,
                    AkAccessor *acc,
                    uint32_t    offset);

char *
ak_id_urlstring(AkHeapAllocator *alc,
                char *id);

#endif /* ak_mesh_util_h */
