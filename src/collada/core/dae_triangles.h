/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_triangles_h_
#define __libassetkit__dae_triangles_h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_triangles(AkXmlState * __restrict xst,
                 void * __restrict memParent,
                 const char * elm,
                 AkTriangleMode mode,
                 AkTriangles ** __restrict dest);

#endif /* __libassetkit__dae_triangles_h_ */
