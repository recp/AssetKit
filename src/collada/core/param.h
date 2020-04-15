/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_param__h_
#define __libassetkit__dae_param__h_

#include "../common.h"

AkNewParam* _assetkit_hide
dae_newparam(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp);

AkParam* _assetkit_hide
dae_param(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

#endif /* __libassetkit__dae_param__h_ */
