/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_spline_h_
#define __libassetkit__dae_spline_h_

#include "../common.h"

AkResult _assetkit_hide
dae_spline(AkXmlState * __restrict xst,
           void * __restrict memParent,
           bool asObject,
           AkSpline ** __restrict dest);

#endif /* __libassetkit__dae_spline_h_ */
