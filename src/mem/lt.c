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

#include "lt.h"
#include <assert.h>
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

static
AkHeapBucket *
ak__heap_lt_find_bucket(uint32_t bucketIndex,
                        AkHeapBucket ** __restrict prev) {
  AkHeapBucket *bucket, *prevBucket;

  bucket     = ak__heap_lt.rootBucket;
  prevBucket = NULL;

  while (bucket && bucket->bucketIndex < bucketIndex) {
    prevBucket = bucket;
    bucket     = bucket->next;
  }

  if (prev)
    *prev = prevBucket;

  if (!bucket || bucket->bucketIndex != bucketIndex)
    return NULL;

  return bucket;
}

static
AkHeapBucket *
ak__heap_lt_find_avail_from(AkHeapBucket * __restrict bucket) {
  while (bucket && AK__LT_BUCKET_IS_FULL(bucket))
    bucket = bucket->next;

  return bucket;
}

static
bool
ak__heap_lt_entry_in_bucket(AkHeapBucket      * __restrict bucket,
                            AkHeapBucketEntry * __restrict entry) {
  return entry >= bucket->heapEntry
         && entry < bucket->heapEntry + ak__heap_lt.bucketSize;
}

static
void
ak__heap_lt_clear_last_used(AkHeapBucket      * __restrict bucket,
                            AkHeapBucketEntry * __restrict entry,
                            bool                            clearBucket) {
  if (!ak__heap_lt.lastUsedEntry)
    return;

  if (ak__heap_lt.lastUsedEntry == entry
      || (clearBucket
          && ak__heap_lt_entry_in_bucket(bucket, ak__heap_lt.lastUsedEntry)))
    ak__heap_lt.lastUsedEntry = NULL;
}

void
ak_heap_lt_init(AkHeap * __restrict initialHeap) {
  assert(initialHeap && "heap cannot be null!");
  ak__heap_bucket.heapEntry = calloc(ak__heap_lt.bucketSize,
                                     sizeof(AkHeapBucketEntry));

  assert(ak__heap_bucket.heapEntry && "malloc failed");

  ak__heap_bucket.heapEntry[0].heap   = initialHeap;
  ak__heap_bucket.heapEntry[0].heapid = 0;

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
  if (!bucket || AK__LT_BUCKET_IS_FULL(bucket)) {
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
  while (++bucket->firstAvailEntry < ak__heap_lt.bucketSize) {
    if (!bucket->heapEntry[bucket->firstAvailEntry].heap)
      break;
  }

  if (AK__LT_BUCKET_IS_FULL(bucket))
    ak__heap_lt.firstAvailBucket = NULL;

  bucket->count++;

  ak__heap_lt.lastUsedEntry = bucketEntry;
}

AkHeap *
ak_heap_lt_find(uint32_t heapid) {
  AkHeapBucket      *bucket;
  AkHeapBucketEntry *entry;
  uint32_t           bucketIndex;
  uint32_t           entryIndex;

  if (ak__heap_lt.lastUsedEntry
      && ak__heap_lt.lastUsedEntry->heap
      && ak__heap_lt.lastUsedEntry->heapid == heapid)
    return ak__heap_lt.lastUsedEntry->heap;

  bucketIndex = heapid / ak__heap_lt.bucketSize;
  entryIndex  = heapid % ak__heap_lt.bucketSize;

  bucket = ak__heap_lt_find_bucket(bucketIndex, NULL);
  if (!bucket)
    return NULL;

  entry = &bucket->heapEntry[entryIndex];
  if (!entry->heap || entry->heapid != heapid)
    return NULL;

  ak__heap_lt.lastUsedEntry = entry;
  return entry->heap;
}

void
ak_heap_lt_remove(uint32_t heapid) {
  AkHeapBucket      *prevBucket;
  AkHeapBucket      *bucket;
  AkHeapBucketEntry *entry;
  uint32_t           bucketIndex;
  uint32_t           entryIndex;
  bool               destroyBucket;

  bucketIndex = heapid / ak__heap_lt.bucketSize;
  entryIndex  = heapid % ak__heap_lt.bucketSize;

  bucket = ak__heap_lt_find_bucket(bucketIndex, &prevBucket);
  if (!bucket)
    return;

  entry = &bucket->heapEntry[entryIndex];
  if (entry && entry->heap && entry->heapid == heapid) {
    memset(&bucket->heapEntry[entryIndex],
           '\0',
           sizeof(AkHeapBucketEntry));
    bucket->count--;

    if (!AK__LT_BUCKET_IS_FULL(bucket))
      bucket->firstAvailEntry = entryIndex;

    destroyBucket = bucket->count < 1 && bucket != &ak__heap_bucket;
    ak__heap_lt_clear_last_used(bucket, entry, destroyBucket);

    if (destroyBucket) {
      if (ak__heap_lt.firstAvailBucket == bucket) {
        ak__heap_lt.firstAvailBucket = ak__heap_lt_find_avail_from(bucket->next);
      }

      if (ak__heap_lt.lastBucket == bucket)
        ak__heap_lt.lastBucket = prevBucket;

      /* we know that prevBucket cannot be null because rootBucket is static */
      prevBucket->next = bucket->next;
      free(bucket->heapEntry);
      free(bucket);

      ak__heap_lt.size--;
    } else {
      if (!ak__heap_lt.firstAvailBucket
          || bucket->bucketIndex < ak__heap_lt.firstAvailBucket->bucketIndex)
        ak__heap_lt.firstAvailBucket = bucket;
    }
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
