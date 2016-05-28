/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_hashtable_h
#define ak_hashtable_h

#include "ak_memory_common.h"

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
  size_t             bucketSize; /* default = 4 */
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
