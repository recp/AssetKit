/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_curve_h
#define dae_brep_curve_h

#include "../common.h"

AkCurve* _assetkit_hide
dae_curve(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

AkCurves* _assetkit_hide
dae_curves(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp);

#endif /* dae_brep_curve_h */
