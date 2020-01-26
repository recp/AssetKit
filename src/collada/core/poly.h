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
  AK_POLYGON_MODE_POLYLIST = 0,
  AK_POLYGON_MODE_POLYGONS = 1
} AkPolygonMode;

AkResult _assetkit_hide
dae_poly(AkXmlState * __restrict xst,
         void       * __restrict memParent,
         const char             *elm,
         AkPolygonMode           mode,
         AkPolygon ** __restrict dest);

#endif /* __libassetkit__dae_polygons_h_ */
