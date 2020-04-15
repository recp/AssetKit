/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_surface_h
#define dae_brep_surface_h

#include "../common.h"

AkSurface* _assetkit_hide
dae_surface(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp);

AkSurfaces* _assetkit_hide
dae_surfaces(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp);

#endif /* dae_brep_surface_h */
