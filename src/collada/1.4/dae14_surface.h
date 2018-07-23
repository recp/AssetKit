/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_surface_h
#define ak_surface_h

#include "../../../include/ak/assetkit.h"
#include "../dae_common.h"
#include "dae14.h"

AkResult _assetkit_hide
dae14_fxSurface(AkXmlState      * __restrict xst,
                void            * __restrict memParent,
                AkDae14Surface ** __restrict dest);

#endif /* ak_surface_h */
