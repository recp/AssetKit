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

/* TODO: use separate rbtree instead of heap's rbtree (optional)
 *       this map impl have many TODOs
 */

#ifndef assetkit_map_h
#define assetkit_map_h
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

AK_EXPORT
void
ak_map_addptr(AkMap *map, void *ptr);

AK_EXPORT
void*
ak_map_find(AkMap *map, void *id);

AK_EXPORT
AkMapItem*
ak_map_findm(AkMap *map, void *id);

AK_EXPORT
void
ak_map_add(AkMap *map,
           void  *value,
           void  *id);

AK_EXPORT
void
ak_multimap_add(AkMap *map,
                void  *value,
                void  *id);

AK_EXPORT
AkMap *
ak_map_new(AkMapCmp cmp);

AK_EXPORT
void
ak_map_destroy(AkMap *map);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_map_h */
