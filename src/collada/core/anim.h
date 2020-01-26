/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_anim_h
#define dae_anim_h

#include "../common.h"

AkResult _assetkit_hide
dae_anim(AkXmlState   * __restrict xst,
         void         * __restrict memParent,
         void        ** __restrict dest);

AkResult _assetkit_hide
dae_animSampler(AkXmlState     * __restrict xst,
                void           * __restrict memParent,
                AkAnimSampler ** __restrict dest);

AkResult _assetkit_hide
dae_channel(AkXmlState  * __restrict xst,
            void        * __restrict memParent,
            AkChannel  ** __restrict dest);

#endif /* dae_anim_h */
