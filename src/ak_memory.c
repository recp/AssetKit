/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_memory.h"
#include "ak_common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __APPLE__
#include <malloc/malloc.h>
#endif

#include <jemalloc/jemalloc.h>

static ak_heap ak__heap = {
  .root       = NULL,
  .trash      = NULL,
  .alloc_zone = NULL,
  .flags      = 0
};

void
ak_heap_init(ak_heap * __restrict heap) {
  if (heap->flags & ak_HEAP_FLAGS_INITIALIZED)
    return;

  heap->root  = NULL;
  heap->trash = NULL;
  heap->flags |= ak_HEAP_FLAGS_INITIALIZED;

#ifdef __APPLE__
  heap->alloc_zone = malloc_create_zone(PAGE_SIZE, 0);
#endif
}

void
ak_heap_destroy(ak_heap * __restrict heap) {
  if (!(heap->flags & ak_HEAP_FLAGS_INITIALIZED))
    return;

#ifdef __APPLE__
  malloc_destroy_zone(heap->alloc_zone);
  heap->root  = NULL;
  heap->trash = NULL;
#else
  ak_heap_cleanup(&ak__heap);
#endif

  if (heap->flags & ak_HEAP_FLAGS_DYNAMIC
      && heap != &ak__heap)
    free(heap);
}

void*
ak_heap_alloc(ak_heap * __restrict heap,
               void * __restrict parent,
               size_t size) {
  ak_heapnode *currNode;
  ak_heapnode *parentNode;

  currNode = je_malloc(ak__heapnd_sz_algnd + size);

  currNode->chld = NULL;

  if (parent) {
    ak_heapnode *chldNode;

    parentNode = ak__alignof(parent);
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

  return ak__alignas(currNode);
}

void*
ak_heap_realloc(ak_heap * __restrict heap,
                 void * __restrict parent,
                 void * __restrict memptr,
                 size_t newsize) {
  ak_heapnode *oldNode;
  ak_heapnode *newNode;

  if (!memptr)
    return ak_malloc(parent, newsize);

  oldNode = ak__alignof(memptr);

  if (newsize == 0) {
    ak_heap_free(heap, oldNode);
    return NULL;
  }

  newNode = realloc(oldNode,
                    ak__heapnd_sz_algnd + newsize);

  return ak__alignas(newNode);
}

void
ak_heap_setp(ak_heap * __restrict heap,
              ak_heapnode * __restrict heapNode,
              ak_heapnode * __restrict newParent) {
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
ak_heap_free(ak_heap * __restrict heap,
              ak_heapnode * __restrict heapNode) {

  /* free all child nodes */
  if (heapNode->chld) {
    ak_heapnode *toFree;
    ak_heapnode *nextFree;

    toFree = heapNode->chld;

    do {
      nextFree = toFree->next;
      if (nextFree)
        nextFree->prev = NULL;

      if (heap->trash == toFree)
        heap->trash = nextFree;

      if (toFree->chld) {
        if (heap->trash) {
          ak_heapnode *lastNode;

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
      
      je_free(toFree);
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
ak_heap_cleanup(ak_heap * __restrict heap) {
  while (heap->root)
    ak_heap_free(heap, heap->root);
}

void
ak_mem_setp(void * __restrict memptr,
             void * __restrict newparent) {
  ak_heap_setp(&ak__heap,
                ak__alignof(memptr),
                ak__alignof(newparent));
}

void*
ak_malloc(void * __restrict parent,
           size_t size) {
  return ak_heap_alloc(&ak__heap,
                        parent,
                        size);
}

void*
ak_calloc(void * __restrict parent,
           size_t size,
           size_t count) {
  void  *memptr;
  size_t memsize;

  memsize = size * count;
  memptr  = ak_heap_alloc(&ak__heap,
                           parent,
                           memsize);
  memset(memptr, '\0', memsize);

  return memptr;
}

void*
ak_realloc(void * __restrict parent,
            void * __restrict memptr,
            size_t newsize) {
  return ak_heap_realloc(&ak__heap,
                          parent,
                          memptr,
                          newsize);
}

char*
ak_strdup(void * __restrict parent,
           const char * __restrict str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak_heap_alloc(&ak__heap,
                           parent,
                           memsize + 1);

  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

void
ak_free(void * __restrict memptr) {
  ak_heap_free(&ak__heap,
                ak__alignof(memptr));
}

void
ak_CONSTRUCTOR
ak__init() {
  ak_heap_init(&ak__heap);
}

void
ak_DESTRUCTOR
ak__cleanup() {
  ak_heap_destroy(&ak__heap);
}
