/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_fx_effect_h
#define dae_fx_effect_h

#include "../common.h"

_assetkit_hide
void*
dae_effect(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp);

_assetkit_hide
AkInstanceEffect*
dae_instEffect(DAEState * __restrict dst,
               xml_t    * __restrict xml,
               void     * __restrict memp);

#endif /* dae_fx_effect_h */
