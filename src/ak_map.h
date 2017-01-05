/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

/* TODO: use separate rbtree instead of heap's rbtree (optional) */

#ifndef ak_map_h
#define ak_map_h

#include "ak_memory_common.h"
#include <stdlib.h>

typedef struct AkMapItem {
  struct AkMapItem *prev;
  struct AkMapItem *next;
  char   data[];
} AkMapItem;

typedef struct AkMap {
  AkHeap    *heap;
  AkMapItem *root;
} AkMap;

typedef AkHeapSrchCmpFn AkMapCmp;

void
ak_map_addptr(AkMap *map, void *ptr);

void
ak_map_add(AkMap *map,
           void  *value,
           size_t size,
           void  *id);

AkMap *
ak_map_new(AkMapCmp cmp);

void
ak_map_destroy(AkMap *map);

#endif /* ak_map_linear_h */
