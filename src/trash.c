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

#include "trash.h"

#include "../include/ak/assetkit.h"
#include "../include/ak/trash.h"
#include "mem/common.h"

#include <stdlib.h>
#include <string.h>

typedef struct AkTrashItem {
  void               *tofree;
  struct AkTrashItem *next;
} AkTrashItem;

static AkHeap ak__trash_heap = {
  .flags = 0
};

#define trash_heap &ak__trash_heap

AkTrashItem *ak__trash = NULL;

static
int
ak__trash_cmp(void *key1, void *key2) {
  if (key1 > key2)
    return 1;
  else if (key1 < key2)
    return -1;

  return 0;
}

AK_EXPORT
void
ak_trash_add(void *mem) {
  AkTrashItem *item;
  AkHeapNode  *found;
  AkResult     ret;

  ret = ak_heap_getNodeById(trash_heap, mem, &found);
  if (ret == AK_OK)
    return;

  item = ak_heap_alloc(trash_heap, NULL, sizeof(*item));
  ak_heap_setId(trash_heap,
                ak__alignof(item),
                mem);

  item->next   = ak__trash;
  item->tofree = mem;
  ak__trash    = item;
}

AK_EXPORT
void
ak_trash_empty() {
  while (ak__trash) {
    AkTrashItem *tofree;
    tofree    = ak__trash;
    ak__trash = ak__trash->next;

    ak_free(tofree->tofree);
    ak_free(tofree);
  }
}

AK_HIDE
void
ak_trash_init() {
  ak_heap_init(trash_heap, NULL, ak__trash_cmp, NULL);
}

AK_HIDE
void
ak_trash_deinit() {
  ak_heap_destroy(trash_heap);
}
