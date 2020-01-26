/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_float_or_param___h_
#define __libassetkit__dae_float_or_param___h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

AkResult _assetkit_hide
dae_floatOrParam(AkXmlState      * __restrict xst,
                 void            * __restrict memParent,
                 const char      * elm,
                 AkFloatOrParam ** __restrict dest);

#endif /* __libassetkit__dae_float_or_param___h_ */
