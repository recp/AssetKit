/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_morph_h
#define dae_morph_h

#include "../common.h"

AkMorph* _assetkit_hide
dae_morph(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

#endif /* dae_morph_h */
