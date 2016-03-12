/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_memory.h"
#include "aio_common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static aio_heap aio__heap = {
  .root = NULL
};

void*
aio_heap_alloc(aio_heap * __restrict heap,
               void * __restrict parent,
               size_t size) {
  aio_heapnode *currNode;
  aio_heapnode *parentNode;

  currNode = malloc(aio__heapnode_size + size);

  currNode->chld = NULL;
  parentNode     = NULL;

  if (parent) {
    aio_heapnode *chldNode;

    parentNode = aio__alignof(parent);
    chldNode   = parentNode->chld;

    parentNode->chld = currNode;
    if (chldNode) {
      chldNode->prev = currNode;
      currNode->next = chldNode;
    } else {
      currNode->next = NULL;
    }

    currNode->prev = parentNode;
  } else {
    if (heap->root) {
      heap->root->prev = currNode;
      currNode->next   = heap->root;
    } else {
      currNode->next = NULL;
    }

    heap->root = currNode;
    currNode->prev = NULL;
  }

  return aio__alignas(currNode);
}

void
aio_heap_freeChld(aio_heap * __restrict heap,
                  aio_heapnode * __restrict heapNode) {
  if (heapNode->chld)
    aio_heap_free(heap, heapNode->chld);

  if (heapNode->next)
    aio_heap_free(heap, heapNode->next);

  heapNode->chld = NULL;
  heapNode->next = NULL;
  heapNode->prev = NULL;

  free(heapNode);
}

void
aio_heap_free(aio_heap * __restrict heap,
              aio_heapnode * __restrict heapNode) {
  if (heapNode->chld)
    aio_heap_freeChld(heap, heapNode->chld);

  if (heapNode->prev)
    heapNode->prev->next = heapNode->next;

  if (heap->root == heapNode)
    heap->root = heapNode->next;

  heapNode->chld = NULL;
  heapNode->next = NULL;

  free(heapNode);
}

void *
aio_malloc(size_t size) {
  return aio_heap_alloc(&aio__heap,
                        NULL,
                        size);
}

void *
aio_calloc(size_t size, size_t count) {
  void  *memptr;
  size_t memsize;

  memsize = size * count;
  memptr  = aio_heap_alloc(&aio__heap,
                           NULL,
                           memsize);
  memset(memptr, '\0', memsize);

  return memptr;
}

void *
aio_realloc(void * __restrict memptr, size_t size) {
  aio_heapnode *oldNode;
  aio_heapnode *newNode;

  if (!memptr)
    return aio_malloc(size);

  if (size == 0) {
    aio_free(memptr);
    return NULL;
  }

  oldNode = aio__alignof(memptr);
  newNode = realloc(oldNode, aio__aligned_node_size + size);

  return aio__alignas(newNode);
}

char *
aio_strdup(const char * __restrict str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = aio_heap_alloc(&aio__heap,
                           NULL,
                           memsize + 1);

  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

void
aio_free(void * __restrict memptr) {
  aio_heapnode *heapNode;
  aio_heapnode *chldNode;

  heapNode = aio__alignof(memptr);
  chldNode = heapNode->chld;

  if (!chldNode)
    goto freeSelf;

  aio_heap_free(&aio__heap, chldNode);

freeSelf:
  if (heapNode->next)
    heapNode->next->prev = heapNode->prev;

  free(heapNode);
}

void
AIO_DESTRUCTOR
aio_cleanup() {
  aio_heap_free(&aio__heap, aio__heap.root);
}
