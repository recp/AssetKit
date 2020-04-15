/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_polygons_h_
#define __libassetkit__dae_polygons_h_

#include "../common.h"

typedef enum AkPolygonMode {
  AK_POLY_POLYLIST = 1,
  AK_POLY_POLYGONS = 2
} AkPolygonMode;

_assetkit_hide
AkPolygon*
dae_poly(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp,
         AkPolygonMode         mode);

#endif /* __libassetkit__dae_polygons_h_ */
