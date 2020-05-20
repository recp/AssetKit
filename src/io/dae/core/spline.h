/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_spline_h
#define dae_spline_h

#include "../common.h"

AkObject* _assetkit_hide
dae_spline(DAEState   * __restrict dst,
           xml_t      * __restrict xml,
           AkGeometry * __restrict geom);

#endif /* dae_spline_h */
