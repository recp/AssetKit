/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"
#include "mem_common.h"
#include "../include/ak/type.h"
#include "type.h"
#include "default/def_type.h"

#include <ds/rb.h>

RBTree *ak__typetree = NULL;

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

AkTypeDesc*
ak_typeDesc(AkTypeId typeId) {
  return rb_find(ak__typetree, (void *)typeId);
}

void
ak_registerType(AkTypeId typeId, AkTypeDesc *desc) {
  rb_insert(ak__typetree, (void *)typeId, desc);
}

void _assetkit_hide
ak_type_init() {
  AkTypeDesc *it;

  ak__typetree = rb_newtree(NULL, ds_cmp_i32p, NULL);

  /* register predefined type descs */
  it = (AkTypeDesc *)ak_def_typedesc();
  if (it) {
    do {
      ak_registerType(it->typeId, it);
      it++;
    } while (it->size > 0);
  }
}

void _assetkit_hide
ak_type_deinit() {
  rb_destroy(ak__typetree);
}
