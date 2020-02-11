/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__dae_fx_image__h_
#define __libassetkit__dae_fx_image__h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

AkImage* _assetkit_hide
dae_fxImage(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp);

AkInstanceBase* _assetkit_hide
dae_fxInstanceImage(DAEState * __restrict dst,
                    xml_t    * __restrict xml,
                    void     * __restrict memp);

#endif /* __libassetkit__dae_fx_image__h_ */
