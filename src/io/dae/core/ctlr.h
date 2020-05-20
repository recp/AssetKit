/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_controller_h
#define dae_controller_h

#include "../common.h"

void* _assetkit_hide
dae_ctlr(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp);

#endif /* dae_controller_h */
