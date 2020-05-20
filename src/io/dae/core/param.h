/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_param_h
#define dae_param_h

#include "../common.h"

AkNewParam* _assetkit_hide
dae_newparam(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp);

AkParam* _assetkit_hide
dae_param(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

#endif /* dae_param_h */
