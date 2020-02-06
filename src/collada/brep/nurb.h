/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_nurbs_h
#define dae_brep_nurbs_h

#include "../common.h"

AkObject* _assetkit_hide
dae_nurbs(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

AkObject* _assetkit_hide
dae_nurbs_surface(DAEState * __restrict dst,
                  xml_t    * __restrict xml,
                  void     * __restrict memp);

#endif /* dae_brep_nurbs_h */
