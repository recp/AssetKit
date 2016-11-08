/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_fx_evaluate_h_
#define __libassetkit__ak_collada_fx_evaluate_h_

#include "../../../include/assetkit.h"
#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_fxEvaluate(AkDaeState * __restrict daestate,
                  void * __restrict memParent,
                  AkEvaluate ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_evaluate_h_ */
