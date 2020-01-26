/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_nurbs_h
#define dae_brep_nurbs_h

#include "../common.h"

AkResult _assetkit_hide
dae_nurbs(AkXmlState * __restrict xst,
          void * __restrict memParent,
          bool asObject,
          AkNurbs ** __restrict dest);

AkResult _assetkit_hide
dae_nurbs_surface(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  bool asObject,
                  AkNurbsSurface ** __restrict dest);

#endif /* dae_brep_nurbs_h */
