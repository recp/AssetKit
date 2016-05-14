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

typedef struct ak_heapnode_s ak_heapnode;
typedef struct ak_heap_s     ak_heap;

typedef enum AkHeapFlags {
  AK_HEAP_FLAGS_NONE        = 0,
  AK_HEAP_FLAGS_INITIALIZED = 1 << 0,
  AK_HEAP_FLAGS_DYNAMIC     = 1 << 1,
} AkHeapFlags;

void
AK_EXPORT
ak_heap_init(ak_heap * __restrict heap);

void
AK_EXPORT
ak_heap_destroy(ak_heap * __restrict heap);

void*
AK_EXPORT
ak_heap_alloc(ak_heap * __restrict heap,
               void * __restrict parent,
               size_t size);

void*
AK_EXPORT
ak_heap_realloc(ak_heap * __restrict heap,
                 void * __restrict parent,
                 void * __restrict memptr,
                 size_t newsize);

void
AK_EXPORT
ak_heap_setp(ak_heap * __restrict heap,
              ak_heapnode * __restrict heapNode,
              ak_heapnode * __restrict newParent);

void
AK_EXPORT
ak_heap_free(ak_heap * __restrict heap,
              ak_heapnode * __restrict heapNode);

void
AK_EXPORT
ak_heap_cleanup(ak_heap * __restrict heap);

void*
AK_EXPORT
ak_malloc(void * __restrict parent,
           size_t size);

void*
AK_EXPORT
ak_calloc(void * __restrict parent,
           size_t size,
           size_t count);

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

AkObject*
AK_EXPORT
ak_objAlloc(void * __restrict memParent,
            size_t typeSize,
            AkEnum typeEnum,
            AkBool zeroed);

AkObject*
AK_EXPORT
ak_objFrom(void * __restrict memptr);

#endif /* __libassetkit__memory__h_ */
