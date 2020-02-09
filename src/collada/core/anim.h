/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_anim_h
#define dae_anim_h

#include "../common.h"

AkAnimation* _assetkit_hide
dae_anim(DAEState * __restrict xst,
         xml_t    * __restrict xml,
         void     * __restrict memp);

AkAnimSampler* _assetkit_hide
dae_animSampler(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp);

AkChannel* _assetkit_hide
dae_channel(DAEState * __restrict dst,
            void     * __restrict xml,
            void     * __restrict memp);

#endif /* dae_anim_h */
