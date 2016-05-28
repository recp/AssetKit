/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_collada_brep_surface_h
#define ak_collada_brep_surface_h

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_surface(AkHeap * __restrict heap,
               void * __restrict memParent,
               xmlTextReaderPtr reader,
               AkSurface ** __restrict dest);

AkResult _assetkit_hide
ak_dae_surfaces(AkHeap * __restrict heap,
                void * __restrict memParent,
                xmlTextReaderPtr reader,
                AkSurfaces ** __restrict dest);

#endif /* ak_collada_brep_surface_h */
