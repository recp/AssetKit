/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_collada_brep_curve_h
#define ak_collada_brep_curve_h

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_curve(AkDaeState * __restrict daestate,
             void * __restrict memParent,
             bool asObject,
             AkCurve ** __restrict dest);

AkResult _assetkit_hide
ak_dae_curves(AkDaeState * __restrict daestate,
              void * __restrict memParent,
              AkCurves ** __restrict dest);

#endif /* ak_collada_brep_curve_h */
