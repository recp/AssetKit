/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__dae_fx_material__h_
#define __libassetkit__dae_fx_material__h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

void* _assetkit_hide
dae_material(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp);

AkBindMaterial* _assetkit_hide
dae_fxBindMaterial(DAEState * __restrict dst,
                   xml_t    * __restrict xml,
                   void     * __restrict memp);

AkInstanceMaterial* _assetkit_hide
dae_fxInstanceMaterial(DAEState * __restrict dst,
                       xml_t    * __restrict xml,
                       void     * __restrict memp);

#endif /* __libassetkit__dae_fx_material__h_ */
