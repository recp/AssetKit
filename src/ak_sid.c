/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_sid.h"
#include "ak_common.h"
#include "ak_memory_common.h"
#include <stdlib.h>
#include <stdio.h>

AK_EXPORT
const char *
ak_sid_get(void *memnode) {
  AkHeapNode *heapNode;
  AkSIDNode  *sidnode;

  heapNode = ak__alignof(memnode);
  sidnode  = ak_heap_ext_sidnode(heapNode);

  if (!sidnode)
    return NULL;

  return sidnode->sid;
}

AK_EXPORT
const char *
ak_sid_geta(void *memnode,
            void *memptr) {
  AkHeapNode *heapNode;
  AkSIDNode  *sidnode;
  char        *ptr;
  uint16_t    off;
  size_t      sidCount;
  size_t      i;

  heapNode = ak__alignof(memnode);
  sidnode  = ak_heap_ext_sidnode(heapNode);

  if (!sidnode)
    return NULL;

  sidCount = *(size_t *)sidnode->sid;
  off      = (char *)memptr - (char *)memnode;
  ptr      = (char *)sidnode->sid + sizeof(size_t);

  for (i = 0; i < sidCount; i++) {
    if (*(uint16_t *)ptr == off)
      return ptr + sizeof(uint16_t);

    ptr += sizeof(uint16_t) + sizeof(const char **);
  }

  return NULL;
}

AK_EXPORT
void
ak_sid_set(void       *memnode,
           const char * __restrict sid) {
  AkSIDNode  *sidnode;
  AkHeapNode *heapNode;
  AkHeap     *heap;
  int         relink;

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);
  relink   = !((heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
                && (heapNode->flags & AK_HEAP_NODE_FLAGS_SID));

  if (relink)
    sidnode = ak_heap_ext_mk_sidnode(heap, heapNode);
  else
    sidnode = ak_heap_ext_sidnode(heapNode);

  sidnode->sid = sid;

  /* TODO: invalidate refs */
}

AK_EXPORT
void
ak_sid_seta(void       *memnode,
            void       *memptr,
            const char * __restrict sid) {
  AkSIDNode  *sidnode;
  AkHeapNode *heapNode;
  AkHeap     *heap;
  char       *sidptr;
  uint16_t    off;
  int         off0;
  int         relink;
  int         itmsize;

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);
  relink   = !((heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
               && (heapNode->flags & AK_HEAP_NODE_FLAGS_SID));

  if (relink)
    sidnode = ak_heap_ext_mk_sidnode(heap, heapNode);
  else
    sidnode = ak_heap_ext_sidnode(heapNode);

  off0    = sizeof(size_t);
  off     = (char *)memptr - (char *)memnode;
  itmsize = sizeof(uint16_t) + sizeof(uintptr_t);

  if (!sidnode->sids) {
    sidnode->sids = heap->allocator->calloc(off0 + itmsize, 1);
  } else {
    size_t newsize;

    newsize  = *(size_t *)((char *)sidnode->sids) + 1;
    newsize  = newsize * itmsize;

    sidnode->sids = heap->allocator->realloc(sidnode->sids,
                                             newsize + off0);
  }

  sidptr = sidnode->sids;
  sidptr += off0 + itmsize * (*(size_t *)sidptr)++;

  *(uint16_t *)sidptr = off;

  sidptr += sizeof(uint16_t);
  *(uintptr_t *)sidptr = (uintptr_t)sid;

  /* TODO: invalidate refs */
}
