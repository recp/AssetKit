/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_accessor_h
#define ak_accessor_h

#include "ak_common.h"

/*!
 * @brief set accessor params with well-defined param names
 *
 * @param acc      Accessor (must be allocated by ak_heap_malloc)
 * @param semantic Common input semantic
 */
void
ak_accessor_setparams(AkAccessor     *acc,
                      AkInputSemantic semantic);

AkAccessor*
ak_accessor_dup(AkAccessor *oldacc);

#endif /* ak_accessor_h */
