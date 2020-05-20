/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_source_h
#define dae_source_h

#include "../common.h"

AkSource* _assetkit_hide
dae_source(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           AkEnum              (*asEnum)(const char *name, size_t nameLen),
           AkTypeId              enumType);

#endif /* dae_source_h */
