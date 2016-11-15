/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_sid.h"
#include "ak_common.h"
#include "ak_memory_common.h"
#include <stdlib.h>
#include <stdio.h>

AK_EXPORT
const char *
ak_sid_get(void *memnode) {
  AkHeapNode *heapNode;
  AkSIDNode  *sidnode;

  heapNode = ak__alignof(memnode);
  sidnode  = ak_heap_ext_sidnode(heapNode);

  if (!sidnode)
    return NULL;

  return sidnode->sid;
}

AK_EXPORT
const char *
ak_sid_geta(void *memnode,
            void *memptr) {
  AkHeapNode *heapNode;
  AkSIDNode  *sidnode;
  char        *ptr;
  uint16_t    off;
  size_t      sidCount;
  size_t      i;

  heapNode = ak__alignof(memnode);
  sidnode  = ak_heap_ext_sidnode(heapNode);

  if (!sidnode)
    return NULL;

  sidCount = *(size_t *)sidnode->sid;
  off      = (char *)memptr - (char *)memnode;
  ptr      = (char *)sidnode->sid + sizeof(size_t);

  for (i = 0; i < sidCount; i++) {
    if (*(uint16_t *)ptr == off)
      return ptr + sizeof(uint16_t);

    ptr += sizeof(uint16_t) + sizeof(const char **);
  }

  return NULL;
}

void _assetkit_hide
ak_sid_link(AkHeapNode *heapNode,
            AkSIDNode * __restrict sidnode) {
  AkHeapNode *hnode_it1, *hnode_it2;
  AkSIDNode  *parentSIDNode, *nextSIDNode;

  parentSIDNode = NULL;
  hnode_it1     = heapNode;

  while (hnode_it1->prev) {
    /* we found near parent */
    if (hnode_it1->prev->chld != hnode_it1) {
      if ((parentSIDNode = ak_heap_ext_sidnode(hnode_it1->prev))) {
        nextSIDNode = parentSIDNode->chld;

        parentSIDNode->chld = sidnode;
        sidnode->prev       = parentSIDNode;
        sidnode->next       = nextSIDNode;
        nextSIDNode->prev   = sidnode;

        /* check for re-link parent-chld ! */

      nxt:
        while (nextSIDNode) {
          hnode_it2 = ak_heap_ext_sidnode_node(nextSIDNode);

          /* find parent */
          while (hnode_it2->prev) {
            /* we found a parent! 
               if parent is current node, link it */
            if (hnode_it2->prev->chld == hnode_it2
                && hnode_it2->prev == heapNode) {
              AkSIDNode *tmp1, *tmp2;

              tmp2              = nextSIDNode->next;
              tmp2->prev        = nextSIDNode->prev;

              tmp1              = sidnode->chld;
              tmp1->prev        = nextSIDNode;
              sidnode->chld     = nextSIDNode;
              nextSIDNode->prev = sidnode;
              nextSIDNode->next = tmp1;

              nextSIDNode       = tmp2;
              goto nxt;
            }

            hnode_it2 = hnode_it2->prev;
          }

          nextSIDNode = nextSIDNode->next;
        }

        break;
      }
    }

    hnode_it1 = hnode_it1->prev;
  };
}

AK_EXPORT
void
ak_sid_set(void       *memnode,
           const char * __restrict sid) {
  AkSIDNode  *sidnode;
  AkHeapNode *heapNode;
  AkHeap     *heap;
  int         relink;

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);
  relink   = !((heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
                && (heapNode->flags & AK_HEAP_NODE_FLAGS_SID));

  if (relink)
    sidnode = ak_heap_ext_mk_sidnode(heap, heapNode);
  else
    sidnode = ak_heap_ext_sidnode(heapNode);

  sidnode->sid = sid;

  if (relink)
    ak_sid_link(heapNode, sidnode);

  /* TODO: invalidate refs */
}

AK_EXPORT
void
ak_sid_seta(void       *memnode,
            void       *memptr,
            const char * __restrict sid) {
  AkSIDNode  *sidnode;
  AkHeapNode *heapNode;
  AkHeap     *heap;
  char       *sidptr;
  uint16_t    off;
  int         off0;
  int         relink;

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);
  relink   = !((heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
               && (heapNode->flags & AK_HEAP_NODE_FLAGS_SID));

  if (relink)
    sidnode = ak_heap_ext_mk_sidnode(heap, heapNode);
  else
    sidnode = ak_heap_ext_sidnode(heapNode);

  off0 = sizeof(size_t);
  off  = (char *)memptr - (char *)memnode;

  if (!sidnode->sids) {
    sidnode->sids = heap->allocator->calloc(off0
                                            + sizeof(uint16_t)
                                            + sizeof(const char *),
                                            1);
  } else {
    size_t newsize;

    newsize  = off0;
    newsize += ((*(size_t *)((char *)sidnode->sids + off0)) + 1)
                  * (newsize * sizeof(uint16_t) + sizeof(const char *));

    sidnode->sids = heap->allocator->realloc(sidnode->sids, newsize);
  }

  sidptr = sidnode->sids;
  sidptr += off0 + (*(size_t *)(sidptr + off0))++;

  *(uint16_t *)sidptr = off;

  sidptr += sizeof(uint16_t);
  *(const char **)sidptr = sid;

  /* TODO: invalidate refs */
}
