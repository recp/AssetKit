/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_light_h
#define dae_light_h

#include "../common.h"

_assetkit_hide
void*
dae_light(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

#endif /* dae_light_h */
