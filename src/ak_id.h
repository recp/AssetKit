/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_id_h
#define ak_id_h

#include "ak_common.h"

void _assetkit_hide
ak_id_newheap(AkHeap * __restrict heap);

_assetkit_hide
const char *
ak_id_gen(AkHeap     * __restrict doc,
          void       * __restrict parentmem,
          const char * __restrict prefix);

#endif /* ak_id_h */
