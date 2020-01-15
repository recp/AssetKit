/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_source_h_
#define __libassetkit__dae_source_h_

#include "../dae_common.h"

AkSource* _assetkit_hide
dae_source(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           AkEnum              (*asEnum)(const char *name),
           uint32_t              enumLen);

#endif /* __libassetkit__dae_source_h_ */
