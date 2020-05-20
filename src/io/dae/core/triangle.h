/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_triangles_h
#define dae_triangles_h

#include "../common.h"

_assetkit_hide
AkTriangles*
dae_triangles(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp,
              AkTriangleMode        mode);

#endif /* dae_triangles_h */
