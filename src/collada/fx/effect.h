/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__dae_fx_effect__h_
#define __libassetkit__dae_fx_effect__h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

AkEffect* _assetkit_hide
dae_effect(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp);

AkInstanceEffect* _assetkit_hide
dae_fxInstanceEffect(DAEState * __restrict dst,
                     xml_t    * __restrict xml,
                     void     * __restrict memp);

#endif /* __libassetkit__dae_fx_effect__h_ */
