/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_evaluate_scene_h_
#define __libassetkit__dae_evaluate_scene_h_

#include "../dae_common.h"

AkEvaluateScene* _assetkit_hide
dae_evaluateScene(DAEState * __restrict dst,
                  xml_t    * __restrict xml,
                  void     * __restrict memParent);

#endif /* __libassetkit__dae_evaluate_scene_h_ */
