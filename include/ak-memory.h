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

typedef struct ak_heap_node_s ak_heap_node;
typedef struct ak_heap_s      ak_heap;

typedef int (*ak_heap_cmp)(void * __restrict key1,
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

void
AK_EXPORT
ak_heap_init(ak_heap * __restrict heap,
             ak_heap_cmp cmp);

void
AK_EXPORT
ak_heap_destroy(ak_heap * __restrict heap);

void*
AK_EXPORT
ak_heap_alloc(ak_heap * __restrict heap,
              void * __restrict parent,
              size_t size,
              bool srch);

void*
AK_EXPORT
ak_heap_realloc(ak_heap * __restrict heap,
                void * __restrict parent,
                void * __restrict memptr,
                size_t newsize);

void
AK_EXPORT
ak_heap_setp(ak_heap * __restrict heap,
             ak_heap_node * __restrict heap_node,
             ak_heap_node * __restrict newParent);

void
AK_EXPORT
ak_heap_free(ak_heap * __restrict heap,
             ak_heap_node * __restrict heap_node);

void
AK_EXPORT
ak_heap_cleanup(ak_heap * __restrict heap);

void *
AK_EXPORT
ak_heap_getId(ak_heap * __restrict heap,
              ak_heap_node * __restrict heap_node);

void
AK_EXPORT
ak_heap_setId(ak_heap * __restrict heap,
              ak_heap_node * __restrict heap_node,
              void * __restrict memId);

AkResult
AK_EXPORT
ak_heap_getMemById(ak_heap * __restrict heap,
                   void * __restrict memId,
                   void ** __restrict dest);

void
ak_heap_printKeys(ak_heap * __restrict heap);

/* default heap helpers */

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
ak_mem_getMemById(void * __restrict memId,
                  void ** __restrict dest);

/* mem wrapper helpers */

AkObject*
AK_EXPORT
ak_objAlloc(void * __restrict memParent,
            size_t typeSize,
            AkEnum typeEnum,
            AkBool zeroed,
            bool srch);

AkObject*
AK_EXPORT
ak_objFrom(void * __restrict memptr);

#endif /* __libassetkit__memory__h_ */
