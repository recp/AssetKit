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

#include "../common.h"
#include "../../include/ak/util.h"
#include <stdio.h>
#include <assert.h>

AK_EXPORT
void
ak_instanceListAdd(AkInstanceList *list,
                   AkInstanceBase *inst) {
  AkHeap             *heap;
  AkInstanceListItem *ili;

  heap = ak_heap_getheap(list);
  ili  = ak_heap_alloc(heap, list, sizeof(*ili));

  ili->instance = inst;
  ili->index    = ++list->lastindex;

  list->count++;

  ili->prev     = list->last;
  ili->next     = NULL;

  if (!list->first)
    list->first = ili;

  if (list->last)
    list->last->next = ili;

  list->last = ili;
}

AK_EXPORT
void
ak_instanceListDel(AkInstanceList *list,
                   AkInstanceListItem *item) {
  if (list->first == item)
    list->first = item->next;

  if (list->last == item)
    list->last = item->prev;

  if (item->prev)
    item->prev->next = item->next;

  if (item->next)
    item->next->prev = item->prev;

  list->count--;

  ak_free(item);
}

AK_EXPORT
void
ak_instanceListEmpty(AkInstanceList *list) {
  AkInstanceListItem *item, *tofree;
  item = list->first;

  while (item) {
    tofree = item;
    item = item->next;
    ak_free(tofree);
  }

  list->first     = NULL;
  list->last      = NULL;
  list->count     = 0;
  list->lastindex = 0;
}

char*
ak_instanceName(AkInstanceListItem *item) {
  AkHeap         *heap;
  AkInstanceBase *inst;
  char           *name, *objId;
  void           *obj;
  uint32_t        indexDigit;
  size_t          idlen;

  inst = item->instance;
  assert(inst);

  heap = ak_heap_getheap(item);

  /* instance name */
  if (inst->name)
    return ak_heap_strdup(heap, item, inst->name);

  /* we can use node.name or node.id for instance name */
  if (inst->node) {
    char *nodeid;

    if (inst->node->name)
      return ak_heap_strdup(heap,
                            item,
                            inst->node->name);

    nodeid = ak_getId(inst->node);
    if (nodeid)
      return ak_heap_strdup(heap, item, nodeid);
  }

  obj = ak_instanceObject(inst);
  if (!obj)
    return NULL;

  objId      = ak_getId(obj);
  idlen      = strlen(objId);

  indexDigit = ak_digitsize(item->index);
  name       = ak_heap_alloc(heap,
                             item,
                             idlen + indexDigit + 2);

  memcpy(name, objId, idlen);
  sprintf(name + idlen, "-%zu", item->index);
  name[idlen + indexDigit + 1] = '\0';

  return name;
}
