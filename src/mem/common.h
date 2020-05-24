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

#ifndef ak_memory_h
#define ak_memory_h

#include "../../include/ak/memory.h"
#include "../common.h"
#include <ds/allocator.h>

#define ak__align_size 8
#define ak__heapnd_sz  offsetof(AkHeapNode, data)

#define ak__align(size) ((size + ak__align_size - 1)                          \
&~ (uintptr_t)(ak__align_size - 1))

#define ak__alignof(p) ((AkHeapNode *)(((char *)p) - ak__heapnd_sz))
#define ak__alignas(m) ((void *)(((char *)m) + ak__heapnd_sz))

#define AK__BST_LEFT  0
#define AK__BST_RIGHT 1

/*
 * Binary Search Tree Node (Red Black)
 */
typedef struct AkHeapSrchNode {
  void                  *key;
  struct AkHeapSrchNode *chld[2];
} AkHeapSrchNode;

struct AkHeapSrchCtx {
  AkHeapSrchNode   *root;
  AkHeapSrchNode   *nullNode;
  AkHeapSrchCmpFn   cmp;
  AkHeapSrchPrintFn print;
};

typedef struct AkSIDNode {
  /*
   | offset0  | sid_ptr0  | ...
   | uint16_t | uintptr_t | ...
   */
  void             *sids;
  void             *refs;
  const char       *sid;
} AkSIDNode;

typedef struct AkUrlNode {
  size_t  len;
  void  **urls;
} AkUrlNode;

#define AK__HEAPNODE(X)                                                       \
  (((AkHeapNodeExt *)((char *)X - offsetof(AkHeapNodeExt, data)))->node)

/*
 - prev - AkHeapNode - next -
               |
    AkHeapNode o AkHeapNodeExt
               |
             chld
               |               */
struct AkHeapNode {
  AkHeapNode *prev; /* parent */
  AkHeapNode *next; /* right  */
  void       *chld; /* left   */
  uint32_t    heapid;
  uint16_t    typeid;
  uint16_t    flags;
  char        data[];
};


/*
 - prev - AkHeapNode - next -
              |
        AkHeapNodeExt - data
              |
            chld
              |

data: data must contain items with these order with these data types
*-----------------------------------------------------------------------------*
| id       | sid     | refc   | extra     | inf       | usr       | url       |
| SrchNode | SidNode | size_t | uintptr_t | uintptr_t | uintptr_t | UrlNode   |

 */
typedef struct AkHeapNodeExt {
  AkHeapNode *node;
  AkHeapNode *chld;
  char        data[];
} AkHeapNodeExt;

struct AkHeap {
  AkHeapAllocator *allocator;
  AkHeapNode      *root;
  AkHeapNode      *trash;
  AkHeapSrchCtx   *srchctx;
  AkHeap          *chld; /* attached heaps, free all with this */
  AkHeap          *next;
  void            *data;
  AkHeap          *idheap;
  uint32_t         heapid;
  AkEnum           flags;
};

void
ak_sid_destroy(AkHeap * __restrict heap,
               AkSIDNode * __restrict snode);

void *
ak_heap_ext_get(AkHeapNode * __restrict hnode,
                uint16_t                flag);

void *
ak_heap_ext_add(AkHeap     * __restrict heap,
                AkHeapNode * __restrict hnode,
                uint16_t                flag);

void
ak_heap_ext_rm(AkHeap     * __restrict heap,
               AkHeapNode * __restrict hnode,
               uint16_t                flag);

void
ak_heap_ext_free(AkHeap     * __restrict heap,
                 AkHeapNode * __restrict hnode);

void _assetkit_hide
ak_freeh(AkHeapNode * __restrict heapNode);

void
ak_mem_init(void);

void
ak_mem_deinit(void);

_assetkit_hide
void
ak_dsSetAllocator(AkHeapAllocator * __restrict alc,
                  DsAllocator     * __restrict dsalc);

#endif /* ak_memory_h */
