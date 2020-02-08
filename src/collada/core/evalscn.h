/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_evaluate_scene_h_
#define __libassetkit__dae_evaluate_scene_h_

#include "../common.h"

AkEvaluateScene* _assetkit_hide
dae_evalScene(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp);

#endif /* __libassetkit__dae_evaluate_scene_h_ */
