/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "accessor.h"
#include "default/semantic.h"
#include <assert.h>

AkAccessor*
ak_accessor_dup(AkAccessor *oldacc) {
  AkHeap      *heap;
  AkAccessor  *acc;

  heap = ak_heap_getheap(oldacc);
  acc  = ak_heap_alloc(heap, ak_mem_parent(oldacc), sizeof(*acc));

  memcpy(acc, oldacc, sizeof(*oldacc));
  ak_setypeid(acc, AKT_ACCESSOR);

  return acc;
}
