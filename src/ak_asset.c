/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

AK_EXPORT
void*
ak_getAssetInfo(void * __restrict obj,
                size_t itemOffset) {
  AkHeapNode *hnode;
  char      **ai;
  void      **found;

  assert(obj && itemOffset < sizeof(AkAssetInf));

  hnode = ak__alignof(obj);

  do {
    if ((ai = ak_heap_ext_get(hnode, AK_HEAP_NODE_FLAGS_INF))
        && (found = (void **)(*ai + itemOffset)))
      return *found;

    hnode = ak_heap_parent(hnode);
    if (!hnode)
      return NULL;
  } while (true);
}

AK_EXPORT
AkCoordSys*
ak_getCoordSys(void * __restrict obj) {
  return ak_getAssetInfo(obj, offsetof(AkAssetInf, coordSys));
}
