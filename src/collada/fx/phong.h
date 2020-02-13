/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_blinn_phong__h_
#define __libassetkit__dae_blinn_phong__h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

AkTechniqueFxCommon* _assetkit_hide
dae_phong(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

#endif /* __libassetkit__dae_blinn_phong__h_ */
