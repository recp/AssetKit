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

#include "data.h"
#include <assert.h>

AkDataContext*
ak_data_new(void   *memparent,
            size_t  nodeitems,
            size_t  itemsize,
            AkCmpFn cmp) {
  AkHeap        *heap;
  AkDataContext *ctx;

  heap = ak_heap_getheap(memparent);
  ctx  = ak_heap_calloc(heap, memparent, sizeof(*ctx));

  ctx->nodesize = nodeitems * itemsize;
  ctx->itemsize = itemsize;
  ctx->heap     = heap;
  ctx->cmp      = cmp;

  return ctx;
}

int
ak_data_append(AkDataContext *dctx, void *item) {
  AkDataChunk *chunk;
  void        *mem;
  size_t       size;

  size = dctx->itemsize;
  assert(dctx->nodesize > size);

  if (dctx->usedsize + size > dctx->size) {
    chunk = ak_heap_alloc(dctx->heap,
                          dctx,
                          sizeof(*chunk) + dctx->nodesize);
    chunk->usedsize = 0;
    chunk->next     = NULL;

    if (dctx->last)
      dctx->last->next = chunk;

    dctx->last  = chunk;
    dctx->size += dctx->nodesize;

    if (!dctx->data)
      dctx->data = chunk;
  } else {
    chunk = dctx->last;
  }

  mem = chunk->data + chunk->usedsize;
  memcpy(mem, item, size);

  chunk->usedsize += size;
  dctx->usedsize  += size;
  dctx->itemcount++;

  return (int)dctx->itemcount - 1;
}

int
ak_data_append_unq(AkDataContext *dctx, void *item) {
  int idx;

  idx = ak_data_exists(dctx, item);
  if (idx != -1)
    return idx;

  return ak_data_append(dctx, item);
}

void
ak_data_walk(AkDataContext *dctx) {
  AkDataChunk *chunk;
  void  *data;
  size_t isz, csz, i;

  if (!dctx->data)
    return;

  isz   = dctx->itemsize;
  chunk = dctx->data;

  while (chunk) {
    csz = dctx->nodesize - chunk->usedsize;
    for (i = isz; i < csz; i += isz) {
      data = chunk->data;
      dctx->walkFn(dctx, data);
    }
    chunk = chunk->next;
  }
}

int
ak_data_exists(AkDataContext *dctx, void *item) {
  AkDataChunk *chunk;
  char  *pmem;
  size_t isz, csz, i;
  int    idx;
  bool   found;

  if (!dctx->data)
    return -1;

  found = false;
  idx   = 0;
  isz   = dctx->itemsize;
  chunk = dctx->data;

  while (chunk) {
    csz  = chunk->usedsize;
    pmem = chunk->data;

    for (i = 0; i < csz; i += isz) {
      if (dctx->cmp(pmem, item) == 0)
        return idx;

      pmem += isz;
      idx++;
    }
    chunk = chunk->next;
  }

  if (!found)
    idx = -1;

  return idx;
}

size_t
ak_data_join(AkDataContext *dctx,
             void          *buff,
             size_t         bytesoff,
             size_t         stride) {
  AkDataChunk *chunk;
  char        *pmem, *data;
  size_t isz, csz, i, count;

  count = 0;

  if (!dctx->data)
    return count;

  isz   = dctx->itemsize;
  chunk = dctx->data;

  pmem = buff;
  
  if (bytesoff > 0)
    pmem += bytesoff;
  
  if (stride == 0)
    stride = isz;

  while (chunk) {
    csz  = chunk->usedsize;
    data = chunk->data;
    for (i = 0; i < csz; i += isz) {
      memcpy(pmem, data, isz);
      pmem += stride;
      data += isz;
      count++;
    }
    chunk = chunk->next;
  }

  return count;
}
