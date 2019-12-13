/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_technique__h_
#define __libassetkit__dae_technique__h_

#include "../dae_common.h"

_assetkit_hide
AkTechnique*
dae_technique(xml_t  * __restrict xml,
              AkHeap * __restrict heap,
              void   * __restrict memparent);

#endif /* __libassetkit__dae_technique__h_ */
