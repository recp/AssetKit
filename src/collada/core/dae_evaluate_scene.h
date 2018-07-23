/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_evaluate_scene_h_
#define __libassetkit__dae_evaluate_scene_h_

#include "../dae_common.h"

AkResult _assetkit_hide
dae_evaluateScene(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkEvaluateScene ** __restrict dest);

#endif /* __libassetkit__dae_evaluate_scene_h_ */
