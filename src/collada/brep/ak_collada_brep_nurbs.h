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
ak_dae_nurbs(AkXmlState * __restrict xst,
             void * __restrict memParent,
             bool asObject,
             AkNurbs ** __restrict dest);

AkResult _assetkit_hide
ak_dae_nurbs_surface(AkXmlState * __restrict xst,
                     void * __restrict memParent,
                     bool asObject,
                     AkNurbsSurface ** __restrict dest);

#endif /* ak_collada_brep_nurbs_h */
