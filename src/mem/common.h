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

#define AK__HEAP_NODE_FLAGS_ALIGNED (1u << 13)

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

typedef struct AkMemoryMapNode {
  struct AkMemoryMapNode *prev;
  struct AkMemoryMapNode *next;
  void                   *mapped;
  size_t                  sized;
} AkMemoryMapNode;

#define AK__HEAPNODE(X)                                                       \
  (((AkHeapNodeExt *)((char *)X - offsetof(AkHeapNodeExt, data)))->node)

/*
 parent - prev - AkHeapNode - next sibling
               |
    AkHeapNode o AkHeapNodeExt
               |
          first child
               |               */
struct AkHeapNode {
  AkHeapNode *parent;
  AkHeapNode *prev; /* left sibling */
  AkHeapNode *next; /* right sibling */
  void       *chld; /* first child */
  uint32_t    heapid;
  uint16_t    typeid;
  uint16_t    flags;
  char        data[];
};

typedef struct AkHeapAlignedPrefix {
  void   *allocation;
  size_t  size;
  size_t  alignment;
} AkHeapAlignedPrefix;

#if UINTPTR_MAX > UINT32_MAX
#  define AK__HEAP_NODE_EXPECTED_SIZE 40
#else
#  define AK__HEAP_NODE_EXPECTED_SIZE 24
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(AkHeapNode) == AK__HEAP_NODE_EXPECTED_SIZE,
               "AkHeapNode header size changed");
_Static_assert(offsetof(AkHeapNode, data) == AK__HEAP_NODE_EXPECTED_SIZE,
               "AkHeapNode payload offset changed");
#else
typedef char AkHeapNodeSizeChanged[
  sizeof(AkHeapNode) == AK__HEAP_NODE_EXPECTED_SIZE ? 1 : -1
];
typedef char AkHeapNodePayloadOffsetChanged[
  offsetof(AkHeapNode, data) == AK__HEAP_NODE_EXPECTED_SIZE ? 1 : -1
];
#endif


/*
        AkHeapNodeExt - data
              |
          first child
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
  AkHeapStats      stats;
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

AK_HIDE
void
ak__init(void);

AK_HIDE
void
ak__cleanup(void);

AK_HIDE
void
ak_freeh(AkHeapNode * __restrict heapNode);

void
ak_mem_init(void);

void
ak_mem_deinit(void);

AK_HIDE
void
ak_dsSetAllocator(AkHeapAllocator * __restrict alc,
                  DsAllocator     * __restrict dsalc);

#endif /* ak_memory_h */
