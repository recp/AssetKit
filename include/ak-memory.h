/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__memory__h_
#define __libassetkit__memory__h_

#include <stdint.h>
#include <sys/types.h>

typedef struct AkObject {
  struct AkObject * next;
  size_t size;
  AkEnum type;
  void * pData;
  char   data[];
} AkObject;

#define ak_objGet(OBJ) OBJ->pData
#define ak_allocator ak_mem_allocator()

typedef struct AkHeapAllocator {
  void *(*malloc)(size_t);
  void *(*calloc)(size_t, size_t);
  void *(*valloc)(size_t size);
  void *(*realloc)(void *, size_t);
  void *(*memalign)(size_t, size_t);
  char *(*strdup)(const char *);
  void (*free)(void *);
  size_t (*size)(const void *);
} AkHeapAllocator;

typedef struct AkHeapSrchCtx AkHeapSrchCtx;
typedef struct AkHeapNode    AkHeapNode;
typedef struct AkHeap        AkHeap;

typedef int (*AkHeapSrchCmp)(void * __restrict key1,
                             void * __restrict key2);

typedef enum AkHeapFlags {
  AK_HEAP_FLAGS_NONE        = 0,
  AK_HEAP_FLAGS_INITIALIZED = 1 << 0,
  AK_HEAP_FLAGS_DYNAMIC     = 1 << 1,
} AkHeapFlags;

typedef enum AkHeapNodeFlags {
  AK_HEAP_NODE_FLAGS_NONE = 0,
  AK_HEAP_NODE_FLAGS_SRCH = 1 << 0,
  AK_HEAP_NODE_FLAGS_RED  = 1 << 1
} AkHeapNodeFlags;

AkHeapAllocator *
AK_EXPORT
ak_heap_allocator(AkHeap * __restrict heap);

AkHeap *
AK_EXPORT
ak_heap_new(AkHeapAllocator *allocator,
            AkHeapSrchCmp cmp);

void
AK_EXPORT
ak_heap_init(AkHeap * __restrict heap,
             AkHeapAllocator *allocator,
             AkHeapSrchCmp cmp);

void
AK_EXPORT
ak_heap_destroy(AkHeap * __restrict heap);

char*
AK_EXPORT
ak_heap_strdup(AkHeap * __restrict heap,
               const char * str);

void*
AK_EXPORT
ak_heap_alloc(AkHeap * __restrict heap,
              void * __restrict parent,
              size_t size,
              bool srch);

void*
AK_EXPORT
ak_heap_calloc(AkHeap * __restrict heap,
               void * __restrict parent,
               size_t size,
               bool srch);

void*
AK_EXPORT
ak_heap_realloc(AkHeap * __restrict heap,
                void * __restrict parent,
                void * __restrict memptr,
                size_t newsize);

void
AK_EXPORT
ak_heap_setp(AkHeap * __restrict heap,
             AkHeapNode * __restrict heapNode,
             AkHeapNode * __restrict newParent);

void
AK_EXPORT
ak_heap_free(AkHeap * __restrict heap,
             AkHeapNode * __restrict heapNode);

void
AK_EXPORT
ak_heap_cleanup(AkHeap * __restrict heap);

void *
AK_EXPORT
ak_heap_getId(AkHeap * __restrict heap,
              AkHeapNode * __restrict heapNode);

void
AK_EXPORT
ak_heap_setId(AkHeap * __restrict heap,
              AkHeapNode * __restrict heapNode,
              void * __restrict memId);

AkResult
AK_EXPORT
ak_heap_getMemById(AkHeap * __restrict heap,
                   void * __restrict memId,
                   void ** __restrict dest);

void
ak_heap_printKeys(AkHeap * __restrict heap);

/* default heap helpers */

AkHeapAllocator *
AK_EXPORT
ak_mem_allocator();

void
ak_mem_printKeys();

void*
AK_EXPORT
ak_malloc(void * __restrict parent,
          size_t size,
          bool srch);

void*
AK_EXPORT
ak_calloc(void * __restrict parent,
          size_t size,
          bool srch);

char*
AK_EXPORT
ak_strdup(void * __restrict parent,
          const char * __restrict str);

void*
AK_EXPORT
ak_realloc(void * __restrict parent,
           void * __restrict memptr,
           size_t newsize);

void
AK_EXPORT
ak_mem_setp(void * __restrict memptr,
            void * __restrict newparent);

void
AK_EXPORT
ak_free(void * __restrict memptr);

void *
AK_EXPORT
ak_mem_getId(void * __restrict memptr);

void
AK_EXPORT
ak_mem_setId(void * __restrict memptr,
             void * __restrict memId);

AkResult
AK_EXPORT
ak_mem_getMemById(void * __restrict ctx,
                  void * __restrict memId,
                  void ** __restrict dest);

/* mem wrapper helpers */

AkObject*
AK_EXPORT
ak_objAlloc(AkHeap * __restrict heap,
            void * __restrict memParent,
            size_t typeSize,
            AkEnum typeEnum,
            AkBool zeroed,
            bool srch);

AkObject*
AK_EXPORT
ak_objFrom(void * __restrict memptr);

#endif /* __libassetkit__memory__h_ */
