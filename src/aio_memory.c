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

#ifdef __APPLE__
#include <malloc/malloc.h>
#endif

static aio_heap aio__heap = {
  .root       = NULL,
  .trash      = NULL,
  .alloc_zone = NULL,
  .flags      = 0
};

void
aio_heap_init(aio_heap * __restrict heap) {
  if (!(heap->flags & AIO_HEAP_FLAGS_INITIALIZED))
    return;

  heap->root  = NULL;
  heap->trash = NULL;
  heap->flags |= AIO_HEAP_FLAGS_INITIALIZED;

#ifdef __APPLE__
  heap->alloc_zone = malloc_create_zone(PAGE_SIZE, 0);
#endif
}

void
aio_heap_destroy(aio_heap * __restrict heap) {
  if (!(heap->flags & AIO_HEAP_FLAGS_INITIALIZED))
    return;

#ifdef __APPLE__
  malloc_destroy_zone(heap->alloc_zone);
  heap->root  = NULL;
  heap->trash = NULL;
#else
  aio_heap_cleanup(&aio__heap);
#endif

  if (heap->flags & AIO_HEAP_FLAGS_DYNAMIC)
    free(heap);
}

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
    aio_heap_freeChld(heap, heapNode->chld);

  if (heapNode->next)
    aio_heap_freeChld(heap, heapNode->next);

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

void
aio_heap_cleanup(aio_heap * __restrict heap) {
  while (heap->root)
    aio_heap_free(heap, heap->root);
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
  aio_heap_free(&aio__heap,
                aio__alignof(memptr));
}

void
AIO_CONSTRUCTOR
aio__init() {
  aio_heap_init(&aio__heap);
}

void
AIO_DESTRUCTOR
aio_cleanup() {
  aio_heap_destroy(&aio__heap);
}
