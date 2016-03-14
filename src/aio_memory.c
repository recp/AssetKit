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

#include <jemalloc/jemalloc.h>

static aio_heap aio__heap = {
  .root       = NULL,
  .trash      = NULL,
  .alloc_zone = NULL,
  .flags      = 0
};

void
aio_heap_init(aio_heap * __restrict heap) {
  if (heap->flags & AIO_HEAP_FLAGS_INITIALIZED)
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

  if (heap->flags & AIO_HEAP_FLAGS_DYNAMIC
      && heap != &aio__heap)
    free(heap);
}

void*
aio_heap_alloc(aio_heap * __restrict heap,
               void * __restrict parent,
               size_t size) {
  aio_heapnode *currNode;
  aio_heapnode *parentNode;

  currNode = malloc(aio__heapnd_sz_algnd + size);

  currNode->chld = NULL;

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

void*
aio_heap_realloc(aio_heap * __restrict heap,
                 void * __restrict parent,
                 void * __restrict memptr,
                 size_t newsize) {
  aio_heapnode *oldNode;
  aio_heapnode *newNode;

  if (!memptr)
    return aio_malloc(parent, newsize);

  oldNode = aio__alignof(memptr);

  if (newsize == 0) {
    aio_heap_free(heap, oldNode);
    return NULL;
  }

  newNode = realloc(oldNode,
                    aio__heapnd_sz_algnd + newsize);

  return aio__alignas(newNode);
}

void
aio_heap_setp(aio_heap * __restrict heap,
              aio_heapnode * __restrict heapNode,
              aio_heapnode * __restrict newParent) {
  if (heapNode->prev)
    heapNode->prev->next = heapNode->next;

  if (heapNode->next)
    heapNode->next->prev = heapNode->prev;

  if (heapNode == heap->root)
    heap->root = heapNode->next;

  if (newParent->chld) {
    newParent->chld->prev = heapNode;
    heapNode->next = newParent->chld;
  }

  newParent->chld = heapNode;
}

void
aio_heap_free(aio_heap * __restrict heap,
              aio_heapnode * __restrict heapNode) {

  /* free all child nodes */
  if (heapNode->chld) {
    aio_heapnode *toFree;
    aio_heapnode *nextFree;

    toFree = heapNode->chld;

    do {
      nextFree = toFree->next;
      if (nextFree)
        nextFree->prev = NULL;

      if (heap->trash == toFree)
        heap->trash = nextFree;

      if (toFree->chld) {
        if (heap->trash) {
          aio_heapnode *lastNode;

          lastNode = toFree->chld;
          while (lastNode->next)
            lastNode = lastNode->next;

          heap->trash->prev = lastNode;
          lastNode->next = heap->trash;
        }

        heap->trash = toFree->chld;
        toFree->chld->prev = NULL;
      }

      /* connect prev and next */
      if (toFree->prev) {
        toFree->prev->next = nextFree;

        if (nextFree)
          nextFree->prev = toFree->prev;
      }
      
      free(toFree);
      toFree = nextFree;

      /* empty trash */
      if (!toFree && heap->trash) {
        toFree = heap->trash;
        heap->trash = nextFree;
      }

    } while (toFree);
    
    heapNode->chld = NULL;
  }

  if (heapNode->prev)
    heapNode->prev->next = heapNode->next;

  if (heap->root == heapNode)
    heap->root = heapNode->next;

  heapNode->next = NULL;
  heapNode->prev = NULL;

  free(heapNode);
}

void
aio_heap_cleanup(aio_heap * __restrict heap) {
  while (heap->root)
    aio_heap_free(heap, heap->root);
}

void
aio_mem_setp(void * __restrict memptr,
             void * __restrict newparent) {
  aio_heap_setp(&aio__heap,
                aio__alignof(memptr),
                aio__alignof(newparent));
}

void*
aio_malloc(void * __restrict parent,
           size_t size) {
  return aio_heap_alloc(&aio__heap,
                        parent,
                        size);
}

void*
aio_calloc(void * __restrict parent,
           size_t size,
           size_t count) {
  void  *memptr;
  size_t memsize;

  memsize = size * count;
  memptr  = aio_heap_alloc(&aio__heap,
                           parent,
                           memsize);
  memset(memptr, '\0', memsize);

  return memptr;
}

void*
aio_realloc(void * __restrict parent,
            void * __restrict memptr,
            size_t newsize) {
  return aio_heap_realloc(&aio__heap,
                          parent,
                          memptr,
                          newsize);
}

char*
aio_strdup(void * __restrict parent,
           const char * __restrict str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = aio_heap_alloc(&aio__heap,
                           parent,
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
aio__cleanup() {
  aio_heap_destroy(&aio__heap);
}
