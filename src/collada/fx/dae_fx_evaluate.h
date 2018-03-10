/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_fx_evaluate_h_
#define __libassetkit__dae_fx_evaluate_h_

#include "../../../include/assetkit.h"
#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_fxEvaluate(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkEvaluate ** __restrict dest);

#endif /* __libassetkit__dae_fx_evaluate_h_ */
