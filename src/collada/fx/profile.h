/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__dae_fx_profile__h_
#define __libassetkit__dae_fx_profile__h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

AkProfile* _assetkit_hide
dae_profile(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp);

#endif /* __libassetkit__dae_fx_profile__h_ */
