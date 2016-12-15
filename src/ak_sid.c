/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include <stdlib.h>
#include <stdio.h>

void * _assetkit_hide
ak_sid_resolve_attr(AkHeapNode *heapNode,
                    uintptr_t   off,
                    const char *attr);

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

  /* mark parents */
  while (heapNode->prev) {
    /* we foound parent */
    if (ak_heap_chld(heapNode->prev) == heapNode)
      heapNode->prev->flags |= AK_HEAP_NODE_FLAGS_SID_CHLD;

    heapNode = heapNode->prev;
  }

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
  int         itmsize;

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);
  relink   = !((heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
               && (heapNode->flags & AK_HEAP_NODE_FLAGS_SID));

  if (relink)
    sidnode = ak_heap_ext_mk_sidnode(heap, heapNode);
  else
    sidnode = ak_heap_ext_sidnode(heapNode);

  off0    = sizeof(size_t);
  off     = (char *)memptr - (char *)memnode;
  itmsize = sizeof(uint16_t) + sizeof(uintptr_t);

  if (!sidnode->sids) {
    sidnode->sids = heap->allocator->calloc(off0 + itmsize, 1);
  } else {
    size_t newsize;

    newsize  = *(size_t *)((char *)sidnode->sids) + 1;
    newsize  = newsize * itmsize;

    sidnode->sids = heap->allocator->realloc(sidnode->sids,
                                             newsize + off0);
  }

  sidptr = sidnode->sids;
  sidptr += off0 + itmsize * (*(size_t *)sidptr)++;

  *(uint16_t *)sidptr = off;

  sidptr += sizeof(uint16_t);
  *(uintptr_t *)sidptr = (uintptr_t)sid;

  /* mark parents */
  while (heapNode->prev) {
    /* we foound parent */
    if (ak_heap_chld(heapNode->prev) == heapNode)
      heapNode->prev->flags |= AK_HEAP_NODE_FLAGS_SID_CHLD;

    heapNode = heapNode->prev;
  }

  /* TODO: invalidate refs */
}

AK_EXPORT
void
ak_sid_dup(void *newMemnode,
           void *oldMemnode) {
  ak_sid_set(newMemnode, ak_heap_strdup(ak_heap_getheap(newMemnode),
                                        newMemnode,
                                        ak_sid_get(oldMemnode)));
}

void
ak_sid_destroy(AkSIDNode * snode) {
  /* TODO delete all sid-s from node */
}

AK_EXPORT
void *
ak_sid_resolve(AkDoc * __restrict doc,
               void  * __restrict sender,
               const char * __restrict sid) {
  AkHeap      *heap;
  AkHeapNode  *idnode, *sidnode, *it;
  char        *siddup, *sid_it, *saveptr;
  void        *found;
  AkHeapNode **buf[2];
  size_t       bufl[2], bufc[2], bufi[2];
  int          bufidx;
  AkResult     r;
  uint16_t     off;

  off     = 0;
  sidnode = NULL;
  found   = NULL;
  heap    = ak_heap_getheap(doc);
  siddup  = strdup(sid);

  sid_it = strtok_r(siddup, "/ \t", &saveptr);

  if (*sid_it == '.') {
    /* .[attr] */
    if (!strchr(sid, '/'))  {
      sid_it++;
      sidnode = sender;
      goto ret;
    }

    /* ./sid.../sid.[attr] */
    idnode = ak__alignof(sender);
  } else {
    r = ak_heap_getNodeById(heap, sid_it, &idnode);
    if (r != AK_OK || !idnode)
      goto err;
  }

  bufi[0] = bufi[1] = bufc[0] = bufc[1] = bufidx = 0;
  bufl[0] = bufl[1] = 4; /* start point */

  buf[0] = malloc(sizeof(*buf[0]) * bufl[0]);
  buf[1] = malloc(sizeof(*buf[0]) * bufl[0]);

  buf[bufidx][bufi[bufidx]] = ak_heap_chld(idnode);

  sid_it = strtok_r(NULL, "/ \t", &saveptr); /* first sid */
  bufc[bufidx] = 1;

  /* breadth-first search */
  while (bufc[bufidx] > 0) {

    it = buf[bufidx][bufi[bufidx]];

    while (it) {
      if (it->flags & AK_HEAP_NODE_FLAGS_SID) {
        AkSIDNode *snode;

        snode = ak_heap_ext_sidnode(it);
        if (snode->sid && strcmp(snode->sid, sid_it) == 0) {
          char *tok;

          sidnode = it;

          /* go for next */
          tok = strtok_r(NULL, "/ \t", &saveptr);
          if (!tok)
            goto ret;

          sid_it = tok;
        }

        /* check attrs */
        if (snode->sids) {
          char  *p, *end;
          size_t c;

          c   = *(size_t *)snode->sids;
          p   = (char *)snode->sids + sizeof(size_t);
          end = p + c * (sizeof(char **) + sizeof(uint16_t));

          while (p != end) {
            off = *(uint16_t *)p;
            p  += sizeof(uint16_t);

            /* found sid in attr */
            if (strcmp(*(char **)p, sid_it) == 0) {
              char *tok;

              sidnode = it;

              /* there is no way to go down */
              tok = strtok_r(NULL, "/ \t", &saveptr);
              if (tok)
                goto err;

              goto ret;
            }

            p += sizeof(char **) + sizeof(uint16_t);
          }
        }
      }

      if (it->flags & AK_HEAP_NODE_FLAGS_SID_CHLD) {
        /* keep all children */
        AkHeapNode *it2;

        it2 = ak_heap_chld(it);
        while (it2) {

          if (bufi[!bufidx] == bufl[!bufidx]) {
            bufl[!bufidx] += 16;
            buf[!bufidx]   = realloc(buf[!bufidx],
                                     bufl[!bufidx]);
          }

          buf[!bufidx][bufi[!bufidx]] = it2;
          bufi[!bufidx]++;
          bufc[!bufidx]++;

          it2 = it2->next;
        }
      }

      it = it->next;
    }

    if (++bufi[bufidx] == bufc[bufidx]) {
      bufc[bufidx] = bufi[bufidx] = bufi[!bufidx] = 0;
      bufidx       = !bufidx;
    }
  }

ret:
  found = ak_sid_resolve_attr(sidnode,
                              off,
                              sid_it);
err:
  free(buf[0]);
  free(buf[1]);
  free(siddup);

  return found;
}

void * _assetkit_hide
ak_sid_resolve_attr(AkHeapNode *heapNode,
                    uintptr_t   off,
                    const char *attr) {
  return heapNode->data;
  return NULL;
}
