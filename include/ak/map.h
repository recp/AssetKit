/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

/* TODO: use separate rbtree instead of heap's rbtree (optional)
 *       this map impl have many TODOs
 */

#ifndef ak_map_h
#define ak_map_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "memory.h"
#include <stdbool.h>

typedef struct AkMapItem {
  struct AkMapItem *prev;
  struct AkMapItem *next;
  void             *data;
  bool              isMapItem;
} AkMapItem;

typedef struct AkMap {
  AkHeap    *heap;
  AkMapItem *root;
} AkMap;

typedef AkHeapSrchCmpFn AkMapCmp;

void
ak_map_addptr(AkMap *map, void *ptr);

void*
ak_map_find(AkMap *map, void *id);

AkMapItem*
ak_map_findm(AkMap *map, void *id);

void
ak_map_add(AkMap *map,
           void  *value,
           void  *id);

void
ak_multimap_add(AkMap *map,
                void  *value,
                void  *id);

AkMap *
ak_map_new(AkMapCmp cmp);

void
ak_map_destroy(AkMap *map);

#ifdef __cplusplus
}
#endif
#endif /* ak_map_linear_h */
