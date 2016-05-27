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
#define ak__heapnd_sz  (sizeof(AkHeapNode*) * 3 + sizeof(AkHeapNodeFlags))

#define ak__align(size) ((size + ak__align_size - 1)                          \
        &~ (uintptr_t)(ak__align_size - 1))

#define ak__alignof(p) ((AkHeapNode *)(((char *)p)-ak__heapnd_sz))
#define ak__alignas(m) ((void *)(((char *)m)+ak__heapnd_sz))

#define AK__BST_LEFT  0
#define AK__BST_RIGHT 1

/*
 * Binary Search Tree Node (Red Black)
 */
typedef struct AkHeapSrchNode {
  void                  *key;
  struct AkHeapSrchNode *chld[2];
} AkHeapSrchNode;

typedef struct AkHeapSrchContext {
  AkHeapSrchNode  *root;
  AkHeapSrchNode  *nullNode;
  AkHeapSrchCmp    cmp;
} AkHeapSrchContext;

#define AK__HEAPNODE(X) \
            ((AkHeapNode *)(((char *)X) + sizeof(AkHeapSrchNode)))

#define AK__HEAPNODE_ISRED(X)  (X->flags & AK_HEAP_NODE_FLAGS_RED)
#define AK__HEAPNODE_MKRED(X)   X->flags |= AK_HEAP_NODE_FLAGS_RED
#define AK__HEAPNODE_MKBLACK(X) X->flags &= ~AK_HEAP_NODE_FLAGS_RED

/*
 case 1:
 
     | AkHeapNode | data |
     ^
  pointer

 case 2:
     | AkHeapSrchNode | AkHeapNode | data |
                      ^
                   pointer
 */
struct AkHeapNode {
  AkHeapNode *prev; /* parent */
  AkHeapNode *next; /* right  */
  AkHeapNode *chld; /* left   */
  AkEnum      flags;
  char        data[];
};

struct AkHeap {
  AkHeapAllocator   *allocator;
  AkHeapNode        *root;
  AkHeapNode        *trash;
  AkHeapSrchContext *srchCtx;
  AkEnum             flags;
};

#endif /* ak_memory_h */
