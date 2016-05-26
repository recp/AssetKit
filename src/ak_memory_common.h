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
#define ak__heapnd_sz  (sizeof(ak_heap_node*) * 3 + sizeof(AkHeapNodeFlags))

#define ak__align(size) ((size + ak__align_size - 1)                          \
        &~ (uintptr_t)(ak__align_size - 1))

#define ak__alignof(p) ((ak_heap_node *)(((char *)p)-ak__heapnd_sz))
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

#define AK__HEAPNODE(X) \
            ((ak_heap_node *)(((char *)X) + sizeof(AkHeapSrchNode)))

#define AK__HEAPNODE_ISRED(X)  (X->flags & AK_HEAP_NODE_FLAGS_RED)
#define AK__HEAPNODE_MKRED(X)   X->flags |= AK_HEAP_NODE_FLAGS_RED
#define AK__HEAPNODE_MKBLACK(X) X->flags &= ~AK_HEAP_NODE_FLAGS_RED

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
  ak_heap_node   *root;
  ak_heap_node   *trash;
#ifdef __APPLE__
  void           *alloc_zone;
#endif
  AkHeapSrchNode *srchRoot;
  AkHeapSrchNode *srchNullNode;

  ak_heap_cmp     cmp;
  AkEnum          flags;
};

#endif /* ak_memory_h */
