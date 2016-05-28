/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_collada_brep_nurbs_h
#define ak_collada_brep_nurbs_h

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_nurbs(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             bool asObject,
             AkNurbs ** __restrict dest);

AkResult _assetkit_hide
ak_dae_nurbs_surface(AkHeap * __restrict heap,
                     void * __restrict memParent,
                     xmlTextReaderPtr reader,
                     bool asObject,
                     AkNurbsSurface ** __restrict dest);

#endif /* ak_collada_brep_nurbs_h */
