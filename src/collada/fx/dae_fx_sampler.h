/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_fx_sampler_h_
#define __libassetkit__dae_fx_sampler_h_

#include "../../../include/assetkit.h"
#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_fxSampler(AkXmlState * __restrict xst,
                 void * __restrict memParent,
                 const char *elm,
                 AkSampler ** __restrict dest);

#endif /* __libassetkit__dae_fx_sampler_h_ */
