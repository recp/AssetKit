/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "../include/ak-type.h"

AkTypeId
ak_typeidh(AkHeapNode * __restrict hnode) {
  return hnode->typeid;
}

AkTypeId
ak_typeid(void * __restrict mem) {
  AkHeapNode *hnode;

  hnode = ak__alignof(mem);
  return hnode->typeid;
}

void
ak_setypeid(void * __restrict mem,
            AkTypeId tid) {
  AkHeapNode *hnode;

  hnode = ak__alignof(mem);
  hnode->typeid = tid;
}

bool
ak_isKindOf(void * __restrict mem,
            void * __restrict other) {
  AkHeapNode *hnode1, *hnode2;

  if (!mem || !other)
    return false;

  hnode1 = ak__alignof(mem);
  hnode2 = ak__alignof(other);

  return hnode1->typeid == hnode2->typeid;
}
