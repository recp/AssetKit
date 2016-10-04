/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "ak_memory_rb.h"
#include "ak_memory_lt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef _WIN32
#  include <jemalloc/jemalloc.h>
#else
# ifndef JEMALLOC_VERSION
#   define je_malloc(size)          malloc(size)
#   define je_calloc(size, count)   calloc(size, count)
#   define je_realloc(ptr, newsize) realloc(ptr, newsize)
#   define je_free(ptr)             free(ptr)
# endif
#endif

static
int
ak__heap_srch_cmp(void * __restrict key1,
                  void * __restrict key2);

static
void
ak__heap_srch_print(void * __restrict key);

static
char*
ak__heap_strdup_def(const char * str);

static char * ak__emptystr = "";

static AkHeapAllocator ak__allocator = {
#ifdef JEMALLOC_VERSION
  .malloc   = je_malloc,
  .calloc   = je_calloc,
  .valloc   = je_valloc,
  .realloc  = je_realloc,
  .memalign = je_posix_memalign,
  .free     = je_free,
#else
  .malloc   = malloc,
  .calloc   = calloc,
  .realloc  = realloc,
  .free     = free,
#ifndef _WIN32
  .valloc   = valloc,
  .memalign = posix_memalign,
#endif
#endif
  .strdup   = ak__heap_strdup_def
};

static AkHeap ak__heap = {
  .allocator  = &ak__allocator,
  .srchctx    = NULL,
  .root       = NULL,
  .trash      = NULL,
  .flags      = 0
};

static
int
ak__heap_srch_cmp(void * __restrict key1,
                  void * __restrict key2) {
  return strcmp((char *)key1, (char *)key2);
}

static
void
ak__heap_srch_print(void * __restrict key) {
  printf("\t%s\n", (const char *)key);
}

static
char*
ak__heap_strdup_def(const char * str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak__heap.allocator->malloc(memsize + 1);
  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

AK_EXPORT
char*
ak_heap_strdup(AkHeap * __restrict heap,
               void * __restrict parent,
               const char * str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak_heap_alloc(heap, parent, memsize + 1, false);
  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

AK_EXPORT
AkHeapAllocator *
ak_heap_allocator(AkHeap * __restrict heap) {
  return heap->allocator;
}

AK_EXPORT
AkHeap *
ak_heap_getheap(void * __restrict memptr) {
  AkHeapNode   *heapNode;
  heapNode = ak__alignof(memptr);
  return ak_heap_lt_find(heapNode->heapid);
}

AK_EXPORT
AkHeap *
ak_heap_new(AkHeapAllocator *allocator,
            AkHeapSrchCmpFn cmp,
            AkHeapSrchPrintFn print) {
  AkHeap *heap;

  heap = je_malloc(sizeof(*heap));
  assert(heap && "malloc failed");

  heap->flags = AK_HEAP_FLAGS_DYNAMIC;
  ak_heap_init(heap, allocator, cmp, print);
  
  return heap;
}

AK_EXPORT
AkResult
ak_heap_attachto(AkHeap * __restrict heap,
                 void * __restrict memptr) {

  heap->data = memptr;
  return AK_OK;
}

AK_EXPORT
void*
ak_heap_attachment(AkHeap * __restrict heap) {
  return heap->data;
}

AK_EXPORT
void
ak_heap_init(AkHeap * __restrict heap,
             AkHeapAllocator *allocator,
             AkHeapSrchCmpFn cmp,
             AkHeapSrchPrintFn print) {
  AkHeapSrchNode    *srchRootNode;
  AkHeapSrchNode    *srchNullNode;
  AkHeapSrchCtx *srchctx;

  size_t srchNodeSize;

  if (heap->flags & AK_HEAP_FLAGS_INITIALIZED)
    return;

  srchctx = je_malloc(sizeof(*srchctx));
  assert(srchctx && "malloc failed");

  srchctx->cmp = cmp ? cmp : ak__heap_srch_cmp;
  srchctx->print = print ? print : ak__heap_srch_print;

  srchNodeSize = sizeof(AkHeapSrchNode) + ak__heapnd_sz;
  srchRootNode = je_calloc(srchNodeSize, 1);
  srchNullNode = je_calloc(srchNodeSize, 1);

  assert(srchRootNode
         && srchNullNode
         && "malloc failed");

  srchRootNode->key = ak__emptystr;
  srchRootNode->chld[AK__BST_LEFT]  = srchNullNode;
  srchRootNode->chld[AK__BST_RIGHT] = srchNullNode;

  srchNullNode->key = ak__emptystr;
  srchNullNode->chld[AK__BST_LEFT]  = srchNullNode;
  srchNullNode->chld[AK__BST_RIGHT] = srchNullNode;

  AK__HEAPNODE(srchRootNode)->flags = AK_HEAP_NODE_FLAGS_SRCH;
  AK__HEAPNODE(srchNullNode)->flags = AK_HEAP_NODE_FLAGS_SRCH;

  /* Real Root is srchRoot-right node */
  srchctx->root     = srchRootNode;
  srchctx->nullNode = srchNullNode;

  heap->root      = NULL;
  heap->trash     = NULL;
  heap->srchctx   = srchctx;
  heap->allocator = allocator ? allocator : &ak__allocator;
  heap->flags    |= AK_HEAP_FLAGS_INITIALIZED;

  if (heap != &ak__heap)
    ak_heap_lt_insert(heap);
}

AK_EXPORT
void
ak_heap_destroy(AkHeap * __restrict heap) {
  if (!(heap->flags & AK_HEAP_FLAGS_INITIALIZED))
    return;

  ak_heap_cleanup(heap);

  je_free(heap->srchctx->root);
  je_free(heap->srchctx->nullNode);
  je_free(heap->srchctx);

  heap->data = NULL;
  ak_heap_lt_remove(heap->heapid);

  if (heap->flags & AK_HEAP_FLAGS_DYNAMIC
      && heap != &ak__heap)
    je_free(heap);
}

AK_EXPORT
void*
ak_heap_alloc(AkHeap * __restrict heap,
              void * __restrict parent,
              size_t size,
              bool srch) {
  char         *chunk;
  AkHeapNode *currNode;
  AkHeapNode *parentNode;
  size_t memsize;

  assert((!parent || heap->heapid == ak__alignof(parent)->heapid)
         && "parent and child mem must use same heap");

  memsize = ak__heapnd_sz + size;
  if (srch)
    memsize += sizeof(AkHeapSrchNode);

  memsize = ak__align(memsize);
  chunk = heap->allocator->malloc(memsize);
  assert(chunk && "malloc failed");

  if (srch) {
    AkHeapSrchNode *srchNode;
    memset(chunk,
           '\0',
           sizeof(AkHeapSrchNode));

    currNode = (AkHeapNode *)(chunk + sizeof(AkHeapSrchNode));
    currNode->flags |= (AK_HEAP_NODE_FLAGS_SRCH | AK_HEAP_NODE_FLAGS_RED);

    srchNode = (AkHeapSrchNode *)chunk;
    srchNode->chld[AK__BST_LEFT]  = heap->srchctx->nullNode;
    srchNode->chld[AK__BST_RIGHT] = heap->srchctx->nullNode;
    srchNode->key = ak__emptystr;
  } else {
    currNode = (AkHeapNode *)chunk;
    currNode->flags = 0;
  }

  currNode->heapid = heap->heapid;
  currNode->chld   = NULL;

  if (parent) {
    AkHeapNode *chldNode;

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

AK_EXPORT
void*
ak_heap_calloc(AkHeap * __restrict heap,
               void * __restrict parent,
               size_t size,
               bool srch) {
  void  *memptr;

  memptr = ak_heap_alloc(heap,
                         parent,
                         size,
                         srch);
  memset(memptr, '\0', size);

  return memptr;
}

AK_EXPORT
void*
ak_heap_realloc(AkHeap * __restrict heap,
                void * __restrict parent,
                void * __restrict memptr,
                size_t newsize) {
  AkHeapNode *oldNode;
  AkHeapNode *newNode;

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

AK_EXPORT
void
ak_heap_setp(AkHeap * __restrict heap,
             AkHeapNode * __restrict heapNode,
             AkHeapNode * __restrict newParent) {
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

AK_EXPORT
void
ak_heap_free(AkHeap * __restrict heap,
             AkHeapNode * __restrict heapNode) {
  AkHeapAllocator * __restrict allocator;
  allocator = heap->allocator;

  /* free all child nodes */
  if (heapNode->chld) {
    AkHeapNode *toFree;
    AkHeapNode *nextFree;

    toFree = heapNode->chld;

    do {
      nextFree = toFree->next;
      if (nextFree)
        nextFree->prev = NULL;

      if (heap->trash == toFree)
        heap->trash = nextFree;

      if (toFree->chld) {
        if (heap->trash) {
          AkHeapNode *lastNode;

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
        AkHeapSrchNode *srchNode;
        srchNode = (AkHeapSrchNode *)((char *)toFree - sizeof(AkHeapSrchNode));

        /* remove it from rb tree */
        if (srchNode->key != ak__emptystr)
          ak_heap_rb_remove(heap->srchctx, srchNode);

        allocator->free(srchNode);
      } else {
        allocator->free(toFree);
      }

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

  if (heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    AkHeapSrchNode *srchNode;
    srchNode = (AkHeapSrchNode *)((char *)heapNode - sizeof(AkHeapSrchNode));

    /* remove it from rb tree */
    if (srchNode->key != ak__emptystr)
      ak_heap_rb_remove(heap->srchctx, srchNode);

    allocator->free(srchNode);
  } else {
    allocator->free(heapNode);
  }
}

AK_EXPORT
void
ak_heap_cleanup(AkHeap * __restrict heap) {
  while (heap->root)
    ak_heap_free(heap, heap->root);
}

AK_EXPORT
void *
ak_heap_getId(AkHeap * __restrict heap,
              AkHeapNode * __restrict heapNode) {

  AK__UNUSED(heap);

  if (heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    AkHeapSrchNode *snode;
    snode = (AkHeapSrchNode *)((char *)heapNode - sizeof(*snode));
    return snode->key;
  }

  return NULL;
}

AK_EXPORT
void
ak_heap_setId(AkHeap * __restrict heap,
              AkHeapNode * __restrict heapNode,
              void * __restrict memId) {
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    AkHeapSrchNode *snode;
    snode = (AkHeapSrchNode *)((char *)heapNode - sizeof(*snode));

    if (!memId) {
      ak_heap_rb_remove(heap->srchctx, snode);
      snode->key = ak__emptystr;
    } else {
      snode->key = memId;
      ak_heap_rb_insert(heap->srchctx, snode);
    }
  }
}

AK_EXPORT
AkResult
ak_heap_getMemById(AkHeap * __restrict heap,
                   void * __restrict memId,
                   void ** __restrict dest) {
  AkHeapNode   *heapNode;
  AkHeapSrchNode *srchNode;

  srchNode = ak_heap_rb_find(heap->srchctx, memId);
  if (!srchNode || srchNode == heap->srchctx->nullNode) {
    *dest = NULL;
    return AK_EFOUND;
  }

  heapNode = AK__HEAPNODE(srchNode);
  *dest = ak__alignas(heapNode);

  return AK_OK;
}

AK_EXPORT
void
ak_heap_printKeys(AkHeap * __restrict heap) {
  ak_heap_rb_print(heap->srchctx);
}

AK_EXPORT
AkHeapAllocator *
ak_mem_allocator() {
  return ak__heap.allocator;
}

AK_EXPORT
void
ak_mem_printKeys() {
  ak_heap_rb_print(ak__heap.srchctx);
}

AK_EXPORT
void *
ak_mem_getId(void * __restrict memptr) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(memptr);
  if (heapNode->heapid == 0)
    heap = &ak__heap;
  else
    heap = ak_heap_lt_find(heapNode->heapid);

  return ak_heap_getId(heap, heapNode);
}

AK_EXPORT
void
ak_mem_setId(void * __restrict memptr,
             void * __restrict memId) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(memptr);
  if (heapNode->heapid == 0)
    heap = &ak__heap;
  else
    heap = ak_heap_lt_find(heapNode->heapid);

  ak_heap_setId(heap,
                heapNode,
                memId);
}

AK_EXPORT
AkResult
ak_mem_getMemById(void * __restrict ctx,
                  void * __restrict memId,
                  void ** __restrict dest) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(ctx);
  if (heapNode->heapid == 0)
    heap = &ak__heap;
  else
    heap = ak_heap_lt_find(heapNode->heapid);

  return ak_heap_getMemById(heap,
                            memId,
                            dest);
}

AK_EXPORT
void
ak_mem_setp(void * __restrict memptr,
            void * __restrict newparent) {
  ak_heap_setp(&ak__heap,
                ak__alignof(memptr),
                ak__alignof(newparent));
}

AK_EXPORT
void*
ak_malloc(void * __restrict parent,
          size_t size,
          bool srch) {
  return ak_heap_alloc(&ak__heap,
                       parent,
                       size,
                       srch);
}

AK_EXPORT
void*
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

AK_EXPORT
void*
ak_realloc(void * __restrict parent,
           void * __restrict memptr,
           size_t newsize) {
  return ak_heap_realloc(&ak__heap,
                          parent,
                          memptr,
                          newsize);
}

AK_EXPORT
char*
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

AK_EXPORT
void
ak_free(void * __restrict memptr) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  heap     = &ak__heap;
  heapNode = ak__alignof(memptr);

  if (heapNode->heapid != heap->heapid)
    heap = ak_heap_lt_find(heapNode->heapid);

  if (!heap)
    return;

  if (heap->data == memptr)
    ak_heap_destroy(heap);
  else
    ak_heap_free(heap, heapNode);
}

AK_EXPORT
AkObject*
ak_objAlloc(AkHeap * __restrict heap,
void * __restrict memParent,
            size_t typeSize,
            AkEnum typeEnum,
            bool zeroed,
            bool srch) {
  AkObject * obj;

  assert(typeSize > 0 && "invalid parameter value");

  obj = ak_heap_alloc(heap,
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

AK_EXPORT
AkObject*
ak_objFrom(void * __restrict memptr) {
  AkObject *obj;
  obj = (void *)((char *)memptr - offsetof(AkObject, data));
  assert(obj->pData == memptr && "invalid cas to AkObject");
  return obj;
}

void
AK_CONSTRUCTOR
ak__init() {
  ak_heap_init(&ak__heap, NULL, NULL, NULL);
  ak_heap_lt_init(&ak__heap);
}

void
AK_DESTRUCTOR
ak__cleanup() {
  ak_heap_destroy(&ak__heap);
  ak_heap_lt_cleanup();
}
