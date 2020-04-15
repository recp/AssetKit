/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "lt.h"
#include <assert.h>
#include <math.h>
#include <string.h>

static AkHeapBucket ak__heap_bucket = {
  .heapEntry       = NULL,
  .firstAvailEntry = 1,
  .count           = 1,
  .bucketIndex     = 0
};

static AkHeapLookupTable ak__heap_lt = {
  .rootBucket       = &ak__heap_bucket,
  .lastBucket       = &ak__heap_bucket,
  .firstAvailBucket = &ak__heap_bucket,
  .size             = 1,
  .bucketSize       = 4,
};

void
ak_heap_lt_init(AkHeap * __restrict initialHeap) {
  assert(initialHeap && "heap cannot be null!");
  ak__heap_bucket.heapEntry = calloc(ak__heap_lt.bucketSize,
                                     sizeof(AkHeapBucketEntry));

  assert(ak__heap_bucket.heapEntry && "malloc failed");

  ak__heap_bucket.heapEntry[0] = (AkHeapBucketEntry){
    .heap   = initialHeap,
    .heapid = 0
  };

  ak__heap_lt.lastUsedEntry = ak__heap_lt.rootBucket->heapEntry;
}

void
ak_heap_lt_insert(AkHeap * __restrict heap) {
  AkHeapBucket      *bucket;
  AkHeapBucketEntry *bucketEntry;
  uint32_t           entryIndex;
  uint32_t           heapid;

  bucket = ak__heap_lt.firstAvailBucket;

  /* all buckets are full */
  if (!bucket || bucket->firstAvailEntry >= ak__heap_lt.bucketSize - 1) {
    bucket = calloc(1, sizeof(*bucket));
    assert(bucket && "malloc failed");

    bucket->heapEntry = calloc(ak__heap_lt.bucketSize,
                               sizeof(*bucket->heapEntry));
    assert(bucket->heapEntry && "malloc failed");

    bucket->bucketIndex = ak__heap_lt.lastBucket->bucketIndex + 1;

    ak__heap_lt.size++;
    ak__heap_lt.lastBucket->next = bucket;
    ak__heap_lt.lastBucket       = bucket;
    ak__heap_lt.firstAvailBucket = bucket;
  }

  entryIndex  = bucket->firstAvailEntry;
  bucketEntry = &bucket->heapEntry[entryIndex];
  bucketEntry->heap = heap;

  heapid = bucket->bucketIndex * ak__heap_lt.bucketSize + entryIndex;
  bucketEntry->heapid = heapid;

  heap->heapid = heapid;

  /* find next avail entry */
  while (bucket->firstAvailEntry++ < ak__heap_lt.bucketSize - 1) {
    assert(bucket->firstAvailEntry < 4);
    if (bucket->heapEntry[bucket->firstAvailEntry].heapid == 0)
      break;
  }

  if (bucket->firstAvailEntry >= ak__heap_lt.bucketSize - 1)
    ak__heap_lt.firstAvailBucket = NULL;

  bucket->count++;

  ak__heap_lt.lastUsedEntry = bucketEntry;
}

AkHeap *
ak_heap_lt_find(uint32_t heapid) {
  AkHeapBucket      *bucket;
  AkHeapBucketEntry *entry;
  int bucketIndex;
  int entryIndex;

  if (heapid >= ak__heap_lt.size * ak__heap_lt.bucketSize)
    return NULL;

  if (ak__heap_lt.lastUsedEntry
      && ak__heap_lt.lastUsedEntry->heapid == heapid)
    return ak__heap_lt.lastUsedEntry->heap;

  bucket      = ak__heap_lt.rootBucket;
  bucketIndex = (int)floor(heapid / ak__heap_lt.bucketSize);
  entryIndex  = heapid % ak__heap_lt.bucketSize;

  for (; bucketIndex > 0; bucketIndex--)
    bucket = bucket->next;

  if (!bucket)
    return NULL;

  entry = &bucket->heapEntry[entryIndex];
  ak__heap_lt.lastUsedEntry = entry;

  return entry->heapid == heapid ? entry->heap : NULL;
}

void
ak_heap_lt_remove(uint32_t heapid) {
  AkHeapBucket      *prevBucket;
  AkHeapBucket      *bucket;
  AkHeapBucketEntry *entry;
  int bucketIndex;
  int entryIndex;

  if (heapid >= ak__heap_lt.size * ak__heap_lt.bucketSize)
    return;

  bucket      = ak__heap_lt.rootBucket;
  prevBucket  = ak__heap_lt.rootBucket;

  bucketIndex = (int)floor(heapid / ak__heap_lt.bucketSize);
  entryIndex  = heapid % ak__heap_lt.bucketSize;

  for (; bucketIndex > 0; bucketIndex--) {
    prevBucket = bucket;
    bucket     = bucket->next;
  }

  if (!bucket)
    return;

  entry = &bucket->heapEntry[entryIndex];
  if (entry) {
    memset(&bucket->heapEntry[entryIndex],
           '\0',
           sizeof(AkHeapBucketEntry));
    bucket->count--;

    if (!AK__LT_BUCKET_IS_FULL(bucket))
      bucket->firstAvailEntry = entryIndex;

    if (bucket->count < 1 && bucket != &ak__heap_bucket) {
      if (ak__heap_lt.firstAvailBucket == bucket) {
        AkHeapBucket *nextAvailBucket;

        nextAvailBucket = bucket->next;
        for (; nextAvailBucket && AK__LT_BUCKET_IS_FULL(nextAvailBucket);)
          nextAvailBucket = bucket->next;

        if (nextAvailBucket)
          ak__heap_lt.firstAvailBucket = nextAvailBucket;
        else
          ak__heap_lt.firstAvailBucket = NULL;
      }

      if (ak__heap_lt.lastBucket == bucket)
        ak__heap_lt.lastBucket = prevBucket;

      /* we know that prevBucket cannot be null because rootBucket is static */
      prevBucket->next = bucket->next;
      free(bucket->heapEntry);
      free(bucket);

      ak__heap_lt.size--;
    }

    if (ak__heap_lt.lastUsedEntry == entry)
      ak__heap_lt.lastUsedEntry = NULL;
  }
}

void
ak_heap_lt_cleanup(void) {
  AkHeapBucket *bucket;
  AkHeapBucket *toFree;

  bucket = ak__heap_lt.rootBucket->next;
  while (bucket) {
    toFree = bucket;
    bucket = bucket->next;

    free(toFree->heapEntry);
    free(toFree);
  }

  free(ak__heap_bucket.heapEntry);
}
