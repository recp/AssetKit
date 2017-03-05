/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_trash.h"

#include "../include/assetkit.h"
#include "../include/ak-trash.h"
#include "ak_memory_common.h"

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

void _assetkit_hide
ak_trash_init() {
  ak_heap_init(trash_heap, NULL, ak__trash_cmp, NULL);
}

void _assetkit_hide
ak_trash_deinit() {
  ak_heap_destroy(trash_heap);
}
