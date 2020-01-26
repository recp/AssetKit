/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_surface_h
#define dae_brep_surface_h

#include "../common.h"

AkResult _assetkit_hide
dae_surface(AkXmlState * __restrict xst,
            void * __restrict memParent,
            AkSurface ** __restrict dest);

AkResult _assetkit_hide
dae_surfaces(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkSurfaces ** __restrict dest);

#endif /* dae_brep_surface_h */
