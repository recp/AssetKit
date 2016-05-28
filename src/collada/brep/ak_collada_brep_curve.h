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
ak_dae_curve(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             bool asObject,
             AkCurve ** __restrict dest);

AkResult _assetkit_hide
ak_dae_curves(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkCurves ** __restrict dest);

#endif /* ak_collada_brep_curve_h */
