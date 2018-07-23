/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_curve_h
#define dae_brep_curve_h

#include "../dae_common.h"

AkResult _assetkit_hide
dae_curve(AkXmlState * __restrict xst,
          void * __restrict memParent,
          bool asObject,
          AkCurve ** __restrict dest);

AkResult _assetkit_hide
dae_curves(AkXmlState * __restrict xst,
           void * __restrict memParent,
           AkCurves ** __restrict dest);

#endif /* dae_brep_curve_h */
