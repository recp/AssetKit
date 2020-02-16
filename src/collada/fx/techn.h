/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_technique_fx__h_
#define __libassetkit__dae_technique_fx__h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

AkTechniqueFx* _assetkit_hide
dae_techniqueFx(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp);

AkTechniqueFxCommon* _assetkit_hide
dae_phong(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

#endif /* __libassetkit__dae_technique_fx__h_ */
