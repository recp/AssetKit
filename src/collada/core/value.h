/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_value__h_
#define __libassetkit__dae_value__h_

#include "../common.h"

AkValue* _assetkit_hide
dae_value(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

void _assetkit_hide
dae_dtype(const char *typeName, AkTypeDesc *type);

#endif /* __libassetkit__dae_value__h_ */
