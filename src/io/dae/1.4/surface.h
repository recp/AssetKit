/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_surface_h
#define ak_surface_h

#include "../common.h"
#include "dae14.h"

AkDae14Surface* _assetkit_hide
dae14_surface(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp);

#endif /* ak_surface_h */
