/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_accessor_h
#define dae_accessor_h

#include "../dae_common.h"

AkResult _assetkit_hide
dae_accessor(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkAccessor ** __restrict dest);

#endif /* dae_accessor_h */
