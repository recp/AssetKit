/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "../include/ak/type.h"
#include "type.h"
#include "default/type.h"

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
