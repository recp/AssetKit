/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __APPLE__
#include <malloc/malloc.h>
#endif

#ifndef _WIN32
#  include <jemalloc/jemalloc.h>
#else
# define je_malloc(size)         malloc(size)
# define je_realloc(size, count) realloc(size, count)
# define je_free(size)           free(size)
#endif

#define ak__align_size 8
#define ak__heapnd_sz  (sizeof(ak_heap_node*) * 3 + sizeof(AkHeapNodeFlags))

#define ak__align(size) ((size + ak__align_size - 1)                          \
        &~ (uintptr_t)(ak__align_size - 1))

#define ak__alignof(p) ((ak_heap_node *)(((char *)p)-ak__heapnd_sz))
#define ak__alignas(m) ((void *)(((char *)m)+ak__heapnd_sz))

/*
 * Binary Search Tree Node
 */
typedef struct ak_heap_snode_s ak_heap_snode;

struct ak_heap_snode_s {
  void          *key;
  ak_heap_snode *left;
  ak_heap_snode *right;
};

/*
 case 1:
 
     | ak_heap_node | data |
     ^
  pointer


 case 2:
     | ak_heap_snode | ak_heap_node | data |
                     ^
                  pointer
 */
struct ak_heap_node_s {
  ak_heap_node *prev; /* parent */
  ak_heap_node *next; /* right  */
  ak_heap_node *chld; /* left   */
  AkEnum        flags;
  char data[];
};

struct ak_heap_s {
  ak_heap_node  *root;
  ak_heap_node  *trash;
#ifdef __APPLE__
  void          *alloc_zone;
#endif
  ak_heap_snode *srch_root;
  ak_heap_cmp    cmp;
  AkEnum         flags;
};

static ak_heap ak__heap = {
  .root       = NULL,
  .trash      = NULL,
  .srch_root  = NULL,
  .alloc_zone = NULL,
  .flags      = 0
};

static
int
ak__heap_srch_cmp(void * __restrict key1,
                  void * __restrict key2) {
  return strcmp(key1, key2);
}

void
AK_EXPORT
ak_heap_init(ak_heap * __restrict heap,
             ak_heap_cmp cmp) {
  if (heap->flags & AK_HEAP_FLAGS_INITIALIZED)
    return;

  heap->root  = NULL;
  heap->trash = NULL;
  heap->flags |= AK_HEAP_FLAGS_INITIALIZED;
  heap->cmp = cmp ? cmp : ak__heap_srch_cmp;

#ifdef __APPLE__
  heap->alloc_zone = malloc_create_zone(PAGE_SIZE, 0);
#endif
}

void
AK_EXPORT
ak_heap_destroy(ak_heap * __restrict heap) {
  if (!(heap->flags & AK_HEAP_FLAGS_INITIALIZED))
    return;

#ifdef __APPLE__
  malloc_destroy_zone(heap->alloc_zone);
  heap->root  = NULL;
  heap->trash = NULL;
#else
  ak_heap_cleanup(&ak__heap);
#endif

  if (heap->flags & AK_HEAP_FLAGS_DYNAMIC
      && heap != &ak__heap)
    free(heap);
}

void*
AK_EXPORT
ak_heap_alloc(ak_heap * __restrict heap,
              void * __restrict parent,
              size_t size,
              bool srch) {
  char         *chunk;
  ak_heap_node *currNode;
  ak_heap_node *parentNode;
  size_t memsize;

  memsize = ak__heapnd_sz + size;
  if (srch)
    memsize += sizeof(ak_heap_snode);

  memsize = ak__align(memsize);
  chunk = je_malloc(memsize);
  assert(chunk && "malloc failed");

  if (srch) {
    memset(chunk,
           '\0',
           sizeof(ak_heap_snode));

    currNode = chunk + sizeof(ak_heap_snode);
    currNode->flags |= AK_HEAP_NODE_FLAGS_SRCH;
  } else {
    currNode = chunk;
    currNode->flags = 0;
  }

  currNode->chld = NULL;

  if (parent) {
    ak_heap_node *chldNode;

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
AK_EXPORT
ak_heap_realloc(ak_heap * __restrict heap,
                void * __restrict parent,
                void * __restrict memptr,
                size_t newsize) {
  ak_heap_node *oldNode;
  ak_heap_node *newNode;

  if (!memptr)
    return ak_heap_alloc(heap,
                         parent,
                         newsize,
                         false);

  oldNode = ak__alignof(memptr);

  if (newsize == 0) {
    ak_heap_free(heap, oldNode);
    return NULL;
  }

  newNode = je_realloc(oldNode,
                       ak__heapnd_sz + newsize);
  assert(newNode && "realloc failed");

  if (heap->root == oldNode)
    heap->root = newNode;

  if (heap->trash == oldNode)
    heap->trash = newNode;

  if (newNode->chld)
    newNode->chld->prev = newNode;

  if (newNode->next)
    newNode->next->prev = newNode;

  if (newNode->prev) {
    if (newNode->prev->chld == oldNode)
      newNode->prev->chld = newNode;

    if (newNode->prev->next == oldNode)
      newNode->prev->next = newNode;
  }

  return ak__alignas(newNode);
}

void
AK_EXPORT
ak_heap_setp(ak_heap * __restrict heap,
             ak_heap_node * __restrict heap_node,
             ak_heap_node * __restrict newParent) {
  if (heap_node->prev)
    heap_node->prev->next = heap_node->next;

  if (heap_node->next)
    heap_node->next->prev = heap_node->prev;

  if (heap_node == heap->root)
    heap->root = heap_node->next;

  if (newParent->chld) {
    newParent->chld->prev = heap_node;
    heap_node->next = newParent->chld;
  }

  newParent->chld = heap_node;
}

void
AK_EXPORT
ak_heap_free(ak_heap * __restrict heap,
             ak_heap_node * __restrict heap_node) {

  /* free all child nodes */
  if (heap_node->chld) {
    ak_heap_node *toFree;
    ak_heap_node *nextFree;

    toFree = heap_node->chld;

    do {
      nextFree = toFree->next;
      if (nextFree)
        nextFree->prev = NULL;

      if (heap->trash == toFree)
        heap->trash = nextFree;

      if (toFree->chld) {
        if (heap->trash) {
          ak_heap_node *lastNode;

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

      if (toFree->flags & AK_HEAP_NODE_FLAGS_SRCH) {
        je_free(toFree - sizeof(ak_heap_snode));
      } else {
        je_free(toFree);
      }

      toFree = nextFree;

      /* empty trash */
      if (!toFree && heap->trash) {
        toFree = heap->trash;
        heap->trash = nextFree;
      }

    } while (toFree);
    
    heap_node->chld = NULL;
  }

  if (heap_node->prev)
    heap_node->prev->next = heap_node->next;

  if (heap->root == heap_node)
    heap->root = heap_node->next;

  heap_node->next = NULL;
  heap_node->prev = NULL;

  if (heap_node->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    je_free(heap_node - sizeof(ak_heap_snode));
  } else {
    je_free(heap_node);
  }
}

void
AK_EXPORT
ak_heap_cleanup(ak_heap * __restrict heap) {
  while (heap->root)
    ak_heap_free(heap, heap->root);
}

void
AK_EXPORT
ak_mem_setp(void * __restrict memptr,
            void * __restrict newparent) {
  ak_heap_setp(&ak__heap,
                ak__alignof(memptr),
                ak__alignof(newparent));
}

void*
AK_EXPORT
ak_malloc(void * __restrict parent,
          size_t size,
          bool srch) {
  return ak_heap_alloc(&ak__heap,
                       parent,
                       size,
                       srch);
}

void*
AK_EXPORT
ak_calloc(void * __restrict parent,
          size_t size,
          bool srch) {
  void  *memptr;

  memptr = ak_heap_alloc(&ak__heap,
                         parent,
                         size,
                         srch);
  memset(memptr, '\0', size);

  return memptr;
}

void*
AK_EXPORT
ak_realloc(void * __restrict parent,
           void * __restrict memptr,
           size_t newsize) {
  return ak_heap_realloc(&ak__heap,
                          parent,
                          memptr,
                          newsize);
}

char*
AK_EXPORT
ak_strdup(void * __restrict parent,
          const char * __restrict str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak_heap_alloc(&ak__heap,
                          parent,
                          memsize + 1,
                          false);

  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

void
AK_EXPORT
ak_free(void * __restrict memptr) {
  ak_heap_free(&ak__heap,
                ak__alignof(memptr));
}

AkObject*
AK_EXPORT
ak_objAlloc(void * __restrict memParent,
            size_t typeSize,
            AkEnum typeEnum,
            AkBool zeroed,
            bool srch) {
  AkObject * obj;

  assert(typeSize > 0 && "invalid parameter value");

  obj = ak_heap_alloc(&ak__heap,
                      memParent,
                      sizeof(*obj) + typeSize,
                      srch);

  obj->size  = typeSize;
  obj->type  = typeEnum;
  obj->next  = NULL;
  obj->pData = (void *)((char *)obj + offsetof(AkObject, data));

  if (zeroed)
    memset(obj->pData, '\0', typeSize);

  return obj;
}

AkObject*
AK_EXPORT
ak_objFrom(void * __restrict memptr) {
  AkObject *obj;
  obj = (void *)((char *)memptr - offsetof(AkObject, data));
  assert(obj->pData == memptr && "invalid cas to AkObject");
  return obj;
}

void
ak_CONSTRUCTOR
ak__init() {
  ak_heap_init(&ak__heap, NULL);
}

void
ak_DESTRUCTOR
ak__cleanup() {
  ak_heap_destroy(&ak__heap);
}
