/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_vertices_h
#define dae_vertices_h

#include "../common.h"

_assetkit_hide
AkVertices*
dae_vert(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp);

#endif /* dae_vertices_h */
