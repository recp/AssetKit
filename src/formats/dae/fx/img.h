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

_assetkit_hide
void*
dae_image(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

_assetkit_hide
AkInstanceBase*
dae_instImage(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp);

#endif /* __libassetkit__dae_fx_image__h_ */
