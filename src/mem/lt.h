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

#ifndef ak_hashtable_h
#define ak_hashtable_h

#include "common.h"

#define AK__LT_BUCKET_IS_FULL(X) (X->count == ak__heap_lt.bucketSize)

typedef struct AkHeapBucketEntry {
  AkHeap  *heap;
  uint32_t heapid;
} AkHeapBucketEntry;

typedef struct AkHeapBucket {
  struct AkHeapBucket *next;
  AkHeapBucketEntry   *heapEntry;
  uint32_t             bucketIndex;
  uint32_t             count;
  uint32_t             firstAvailEntry;
} AkHeapBucket;

typedef struct AkHeapLookupTable {
  AkHeapBucket      *rootBucket;
  AkHeapBucket      *lastBucket;
  AkHeapBucket      *firstAvailBucket;
  AkHeapBucketEntry *lastUsedEntry; /* cache last */
  size_t             size;
  uint32_t           bucketSize; /* default = 4 */
} AkHeapLookupTable;

void
ak_heap_lt_init(AkHeap * __restrict initialHeap);

AkHeap *
ak_heap_lt_find(uint32_t heapid);

void
ak_heap_lt_remove(uint32_t heapid);

void
ak_heap_lt_insert(AkHeap * __restrict heap);

void
ak_heap_lt_cleanup(void);

#endif /* ak_hashtable_h */
