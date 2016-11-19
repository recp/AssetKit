/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_memory_h
#define ak_memory_h

#include "ak_common.h"

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

  struct AkSIDNode *prev;
  struct AkSIDNode *chld;
  struct AkSIDNode *next;
} AkSIDNode;

#define AK__HEAPNODE(X)                                                       \
  (((AkHeapNodeExt *)((char *)X - offsetof(AkHeapNodeExt, data)))->node)

#define AK__HEAPEXTNODE(X)                                                    \
  ((AkHeapSrchNode *)((char *)X - sizeof(AkHeapNodeExt)))

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
  uint16_t    heapid;
  uint16_t    typeid;
  uint16_t    depth;
  uint16_t    flags;
  char        data[];
};

/*
 - prev - AkHeapNode - next -
              |
        AkHeapNodeExt
              |
            chld
              |               */
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
  uint16_t         heapid;
  AkEnum           flags;
};

void
ak_heap_ext_setid(AkHeap * __restrict heap,
                  AkHeapNode * __restrict heapNode,
                  void * __restrict memId);

void
ak_heap_ext_unsetid(AkHeap * __restrict heap,
                    AkHeapNode * __restrict heapNode);

AkSIDNode *
ak_heap_ext_mk_sidnode(AkHeap * __restrict heap,
                       AkHeapNode * __restrict heapNode);

AkSIDNode *
ak_heap_ext_sidnode(AkHeapNode * __restrict heapNode);

AkHeapNode *
ak_heap_ext_sidnode_node(AkSIDNode * __restrict sidnode);

void
ak_mem_init();

void
ak_mem_deinit();

#endif /* ak_memory_h */
