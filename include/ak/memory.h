/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef assetkit_memory_h
#define assetkit_memory_h

#include <stdint.h>
#include <sys/types.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AkObject {
  struct AkObject * next;
  size_t size;
  AkEnum type;
  void * pData;
  char   data[];
} AkObject;

#define ak_objGet(OBJ) ((OBJ)->pData)
#define ak_allocator ak_mem_allocator()

typedef struct AkHeapAllocator {
  void  *(*malloc)(size_t);
  void  *(*calloc)(size_t, size_t);
  void *(*realloc)(void *, size_t);
  int  (*memalign)(void **, size_t, size_t);
  char  *(*strdup)(const char *);
  void     (*free)(void *);
  size_t   (*size)(const void *);
} AkHeapAllocator;

typedef struct AkHeapSrchCtx AkHeapSrchCtx;
typedef struct AkHeapNode    AkHeapNode;
typedef struct AkHeap        AkHeap;
struct AkURL;

typedef int (*AkHeapSrchCmpFn)(void * __restrict key1,
                               void * __restrict key2);

typedef void (*AkHeapSrchPrintFn)(void * __restrict key);

typedef enum AkHeapFlags {
  AK_HEAP_FLAGS_NONE        = 0,
  AK_HEAP_FLAGS_INITIALIZED = 1 << 0,
  AK_HEAP_FLAGS_DYNAMIC     = 1 << 1,
} AkHeapFlags;

typedef enum AkHeapNodeFlags {
  AK_HEAP_NODE_FLAGS_NONE      = 0,       /* plain node                      */
  AK_HEAP_NODE_FLAGS_HEAP_CHLD = 1 << 0,  /* node has attached heaps         */
  AK_HEAP_NODE_FLAGS_EXT       = 1 << 1,  /* node has one of:                */
  AK_HEAP_NODE_FLAGS_SID_CHLD  = 1 << 2,  /* least one of children has sid   */
  AK_HEAP_NODE_FLAGS_RED       = 1 << 3,  /* RBtree color bit                */
  AK_HEAP_NODE_FLAGS_SRCH      = 1 << 4,  /* node has an id                  */
  AK_HEAP_NODE_FLAGS_SID       = 1 << 5,  /* memory node or its attr has sid */
  AK_HEAP_NODE_FLAGS_REFC      = 1 << 6,  /* node is reference counted       */
  AK_HEAP_NODE_FLAGS_EXTRA     = 1 << 7,  /* node has <extra> element        */
  AK_HEAP_NODE_FLAGS_INF       = 1 << 8,  /* node has <asset> element        */
  AK_HEAP_NODE_FLAGS_URL       = 1 << 9,  /* node has retained mem via url   */
  AK_HEAP_NODE_FLAGS_USR       = 1 << 10, /* user data                       */
  AK_HEAP_NODE_FLAGS_USRF      = 1 << 11, /* user data must be freed         */
  AK_HEAP_NODE_FLAGS_MMAP      = 1 << 12, /* attached mmap-ed memory list    */
  AK_HEAP_NODE_FLAGS_SID_NODE  = AK_HEAP_NODE_FLAGS_SID,
  AK_HEAP_NODE_FLAGS_EXT_ALL   = AK_HEAP_NODE_FLAGS_EXT
                               | AK_HEAP_NODE_FLAGS_SRCH
                               | AK_HEAP_NODE_FLAGS_SID
                               | AK_HEAP_NODE_FLAGS_REFC
                               | AK_HEAP_NODE_FLAGS_EXTRA
                               | AK_HEAP_NODE_FLAGS_INF
                               | AK_HEAP_NODE_FLAGS_URL
                               | AK_HEAP_NODE_FLAGS_USR
                               | AK_HEAP_NODE_FLAGS_USRF
                               | AK_HEAP_NODE_FLAGS_MMAP,
  AK_HEAP_NODE_FLAGS_EXT_FRST = AK_HEAP_NODE_FLAGS_SRCH
} AkHeapNodeFlags;

AK_EXPORT
AkHeapAllocator *
ak_heap_allocator(AkHeap * __restrict heap);

AK_EXPORT
AkHeap *
ak_heap_getheap(void * __restrict memptr);

AK_EXPORT
AkHeap *
ak_heap_default(void);

AK_EXPORT
AkHeap *
ak_heap_new(AkHeapAllocator *allocator,
            AkHeapSrchCmpFn cmp,
            AkHeapSrchPrintFn print);

AK_EXPORT
void
ak_heap_attach(AkHeap * __restrict parent,
               AkHeap * __restrict chld);

AK_EXPORT
void
ak_heap_dettach(AkHeap * __restrict parent,
                AkHeap * __restrict chld);

AK_EXPORT
void
ak_heap_setdata(AkHeap * __restrict heap,
                void   * __restrict memptr);

AK_EXPORT
void*
ak_heap_data(AkHeap * __restrict heap);

AK_EXPORT
void
ak_heap_init(AkHeap          * __restrict heap,
             AkHeapAllocator * __restrict allocator,
             AkHeapSrchCmpFn              cmp,
             AkHeapSrchPrintFn            print);

AK_EXPORT
void
ak_heap_destroy(AkHeap * __restrict heap);

AK_EXPORT
char*
ak_heap_strdup(AkHeap * __restrict heap,
               void * __restrict parent,
               const char * str);

AK_EXPORT
char*
ak_heap_strndup(AkHeap * __restrict heap,
                void   * __restrict parent,
                const char * str,
                size_t       size);

AK_EXPORT
void*
ak_heap_alloc(AkHeap * __restrict heap,
              void   * __restrict parent,
              size_t size);

AK_EXPORT
void*
ak_heap_calloc(AkHeap * __restrict heap,
               void   * __restrict parent,
               size_t size);

AK_EXPORT
void*
ak_heap_realloc(AkHeap * __restrict heap,
                void   * __restrict parent,
                void   * __restrict memptr,
                size_t newsize);

AK_EXPORT
void *
ak_heap_chld(AkHeapNode *heapNode);

AK_EXPORT
void
ak_heap_chld_set(AkHeapNode * __restrict heapNode,
                 AkHeapNode * __restrict chldNode);

AK_EXPORT
AkHeapNode *
ak_heap_parent(AkHeapNode *heapNode);

AK_EXPORT
void
ak_heap_setp(AkHeapNode * __restrict heapNode,
             AkHeapNode * __restrict newParent);

AK_EXPORT
void
ak_heap_moveh(AkHeapNode * __restrict heapNode,
              AkHeap     * __restrict newheap);

AK_EXPORT
void
ak_heap_setpm(void   * __restrict memptr,
              void   * __restrict newparent);

AK_EXPORT
void
ak_heap_free(AkHeap     * __restrict heap,
             AkHeapNode * __restrict heapNode);

AK_EXPORT
void
ak_heap_cleanup(AkHeap * __restrict heap);

AK_EXPORT
void *
ak_heap_getId(AkHeap     * __restrict heap,
              AkHeapNode * __restrict heapNode);

AK_EXPORT
void
ak_heap_setId(AkHeap     * __restrict heap,
              AkHeapNode * __restrict heapNode,
              void       * __restrict memId);

AK_EXPORT
AkResult
ak_heap_getNodeById(AkHeap      * __restrict heap,
                    void        * __restrict memId,
                    AkHeapNode ** __restrict dest);

AK_EXPORT
AkResult
ak_heap_getNodeByURL(AkHeap       * __restrict heap,
                     struct AkURL * __restrict url,
                     AkHeapNode  ** __restrict dest);

AK_EXPORT
AkResult
ak_heap_getMemById(AkHeap * __restrict heap,
                   void   * __restrict memId,
                   void  ** __restrict dest);

AK_EXPORT
void*
ak_heap_setUserData(AkHeap * __restrict heap,
                    void   * __restrict mem,
                    void   * __restrict userData);

AK_EXPORT
int
ak_heap_refc(AkHeapNode * __restrict heapNode);

AK_EXPORT
int
ak_heap_retain(AkHeapNode * __restrict heapNode);

AK_EXPORT
void
ak_heap_release(AkHeapNode * __restrict heapNode);

AK_EXPORT
void
ak_heap_printKeys(AkHeap * __restrict heap);

/* default heap helpers */

AK_EXPORT
AkHeap*
ak_attachedHeap(void * __restrict memptr);

AK_EXPORT
void
ak_setAttachedHeap(void   * __restrict memptr,
                   AkHeap * __restrict heap);

AK_EXPORT
AkHeapAllocator *
ak_mem_allocator(void);

AK_EXPORT
void
ak_mem_printKeys(void);

AK_EXPORT
void*
ak_malloc(void * __restrict parent,
          size_t size);

AK_EXPORT
void*
ak_calloc(void * __restrict parent,
          size_t size);

AK_EXPORT
char*
ak_strdup(void * __restrict parent,
          const char * __restrict str);

AK_EXPORT
void*
ak_realloc(void * __restrict parent,
           void * __restrict memptr,
           size_t newsize);

AK_EXPORT
void
ak_mem_setp(void * __restrict memptr,
            void * __restrict newparent);

AK_EXPORT
void *
ak_mem_parent(void *mem);

AK_EXPORT
void
ak_free(void * __restrict memptr);

AK_EXPORT
void *
ak_mem_getId(void * __restrict memptr);

AK_EXPORT
void
ak_mem_setId(void * __restrict memptr,
             void * __restrict memId);

AK_EXPORT
AkResult
ak_mem_getMemById(void * __restrict ctx,
                  void * __restrict memId,
                  void ** __restrict dest);

AK_EXPORT
int
ak_refc(void * __restrict mem);

AK_EXPORT
int
ak_retain(void * __restrict mem);

AK_EXPORT
void
ak_release(void * __restrict mem);

/* mem wrapper helpers */

AK_EXPORT
AkObject*
ak_objAlloc(AkHeap * __restrict heap,
            void * __restrict memParent,
            size_t typeSize,
            AkEnum typeEnum,
            bool zeroed);

AK_EXPORT
void*
ak_userData(void * __restrict mem);

AK_EXPORT
void*
ak_setUserData(void * __restrict mem, void * __restrict userData);

AK_EXPORT
AkObject*
ak_objFrom(void * __restrict memptr);

AK_EXPORT
void*
ak_mmap_rdonly(int fd, size_t size);

AK_EXPORT
void
ak_unmap(void *file, size_t size);

AK_EXPORT
void
ak_mmap_attach(void * __restrict obj, void * __restrict mapped, size_t sized);

AK_EXPORT
void
ak_unmmap_attached(void * __restrict obj);

#ifdef __cplusplus
}
#endif

#endif /* assetkit_memory_h */
