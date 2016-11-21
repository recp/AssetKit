/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "ak_memory_rb.h"

void
ak_heap_ext_setid(AkHeap * __restrict heap,
                  AkHeapNode * __restrict heapNode,
                  void * __restrict memId) {
  AkHeapAllocator *alc;
  AkHeapNodeExt   *extNode;
  AkHeapSrchNode  *snode;
  AkHeapNodeExt   *extNodeR;
  AkSIDNode       *sidNode;
  size_t           snodeSize;

  alc       = heap->allocator;
  snodeSize = sizeof(AkHeapSrchNode);

  if (!(heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)) {
    extNode        = alc->malloc(sizeof(*extNode) + snodeSize);
    extNode->node  = heapNode;
    extNode->chld  = heapNode->chld;
    heapNode->chld = extNode;

    snode = (AkHeapSrchNode *)extNode->data;
    goto done;
  }

  extNode = heapNode->chld;

  if (heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    snode = (AkHeapSrchNode *)extNode->data;
    ak_heap_rb_remove(heap->srchctx, snode);
    goto done;
  }

  sidNode  = (AkSIDNode *)(extNode->data + 1);
  extNodeR = alc->realloc(extNode,
                          sizeof(*extNodeR)
                          + snodeSize
                          + sizeof(AkSIDNode)
                          + 1);

  if (heapNode->flags & AK_HEAP_NODE_FLAGS_SID) {
    memmove(extNodeR + snodeSize,
            extNodeR,
            sizeof(AkSIDNode) + 1);
    extNodeR->data[snodeSize] = 1;

    /* TODO: update refs */
    heapNode->chld = extNodeR;
  }

  snode = (AkHeapSrchNode *)extNodeR->data;

done:
  heapNode->flags |= (AK_HEAP_NODE_FLAGS_EXT
                      | AK_HEAP_NODE_FLAGS_SRCH
                      | AK_HEAP_NODE_FLAGS_RED);

  snode->chld[AK__BST_LEFT]  = heap->srchctx->nullNode;
  snode->chld[AK__BST_RIGHT] = heap->srchctx->nullNode;
  snode->key                 = memId;

  ak_heap_rb_insert(heap->srchctx, snode);
}

void
ak_heap_ext_unsetid(AkHeap * __restrict heap,
                    AkHeapNode * __restrict heapNode) {
  AkHeapAllocator *alc;
  AkHeapNodeExt   *extNode;
  AkHeapNodeExt   *extNodeR;
  AkSIDNode       *sidNode;
  size_t           snodeSize;

  alc       = heap->allocator;
  snodeSize = sizeof(AkHeapSrchNode);

  if (!(heapNode->flags
        & (AK_HEAP_NODE_FLAGS_EXT | AK_HEAP_NODE_FLAGS_SRCH))) {
    heapNode->flags &= ~AK_HEAP_NODE_FLAGS_RED;
    return;
  }

  extNode = heapNode->chld;
  ak_heap_rb_remove(heap->srchctx,
                    (AkHeapSrchNode *)extNode->data);

  if (!(heapNode->flags & AK_HEAP_NODE_FLAGS_SID)) {
    heapNode->chld = extNode->chld;
    heapNode->flags &= ~(AK_HEAP_NODE_FLAGS_SRCH
                         | AK_HEAP_NODE_FLAGS_EXT
                         | AK_HEAP_NODE_FLAGS_RED);
    alc->free(extNode);
    return;
  }

  sidNode  = (AkSIDNode *)(extNode->data + snodeSize + 1);
  extNodeR = alc->realloc(extNode,
                          sizeof(*extNodeR)
                          + sizeof(AkSIDNode)
                          + 1);

  if (heapNode->flags & AK_HEAP_NODE_FLAGS_SID) {
    memmove(extNodeR - snodeSize,
            extNodeR,
            sizeof(AkSIDNode) + 1);

    extNodeR->data[0] = 0;

    /* TODO: update refs */
    heapNode->chld = extNodeR;
  }

  heapNode->flags &= ~(AK_HEAP_NODE_FLAGS_SRCH | AK_HEAP_NODE_FLAGS_RED);
}

AkHeapNode *
ak_heap_ext_sidnode_node(AkSIDNode * __restrict sidnode) {
  AkHeapNodeExt *extNode;
  char          *ptr;

  ptr = ((char *)sidnode) - 1 - sizeof(AkHeapNodeExt);
  if (*(((char *)sidnode) - 1))
    ptr -= sizeof(AkHeapSrchNode);

  extNode = (AkHeapNodeExt *)ptr;
  return extNode->node;
}

AkSIDNode *
ak_heap_ext_sidnode(AkHeapNode * __restrict heapNode) {
  if ((heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
      && (heapNode->flags & AK_HEAP_NODE_FLAGS_SID)) {
    AkHeapNodeExt *extNode;

    extNode = heapNode->chld;
    if (!(heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH))
      return (AkSIDNode *)(extNode->data + 1);

    return (AkSIDNode *)(extNode->data + sizeof(AkHeapSrchNode) + 1);
  }

  return NULL;
}

AkSIDNode *
ak_heap_ext_mk_sidnode(AkHeap * __restrict heap,
                       AkHeapNode * __restrict heapNode) {
  AkHeapAllocator *alc;
  AkHeapNodeExt   *extNode;
  AkSIDNode       *sidNode;
  size_t           snodeSize;

  alc         = heap->allocator;
  snodeSize   = sizeof(AkHeapSrchNode);

  if (!(heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)) {
    extNode        = alc->calloc(sizeof(*extNode)
                                 + sizeof(AkSIDNode)
                                 + 1,
                                 1);
    extNode->node  = heapNode;
    extNode->chld  = heapNode->chld;
    heapNode->chld = extNode;

    extNode->data[0] = 0;
    sidNode = (AkSIDNode *)(extNode->data + 1);
    goto done;
  }

  extNode = heapNode->chld;

  if (!(heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH)) {
    sidNode = (AkSIDNode *)(extNode->data + 1);
    goto done;
  }

  if (!(heapNode->flags & AK_HEAP_NODE_FLAGS_SID))
    extNode = alc->realloc(extNode,
                           sizeof(*extNode)
                           + sizeof(AkHeapSrchNode)
                           + sizeof(AkSIDNode)
                           + 1);

  sidNode = (AkSIDNode *)(extNode->data + snodeSize + 1);
  extNode->data[sizeof(AkHeapSrchNode)] = 1;

done:
  heapNode->flags |= (AK_HEAP_NODE_FLAGS_EXT | AK_HEAP_NODE_FLAGS_SID);
  return sidNode;
}
