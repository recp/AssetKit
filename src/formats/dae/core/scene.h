/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_scene_h_
#define __libassetkit__dae_scene_h_

#include "../common.h"

void* _assetkit_hide
dae_vscene(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp);

AkInstanceBase* _assetkit_hide
dae_instVisualScene(DAEState * __restrict dst,
                    xml_t    * __restrict xml,
                    void     * __restrict memp);
void _assetkit_hide
dae_scene(DAEState * __restrict dst,
          xml_t    * __restrict xml);

#endif /* __libassetkit__dae_scene_h_ */
