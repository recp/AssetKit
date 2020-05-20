/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_value_h
#define dae_value_h

#include "../common.h"

AkValue* _assetkit_hide
dae_value(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

void _assetkit_hide
dae_dtype(const char *typeName, AkTypeDesc *type);

#endif /* dae_value_h */
