/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "ak_sid.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

AkHeap ak__sidconst_heap = {
  .flags = 0
};

#define sidconst_heap &ak__sidconst_heap

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
  sidnode  = ak_heap_ext_get(heapNode, AK_HEAP_NODE_FLAGS_SID);

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
  sidnode  = ak_heap_ext_get(heapNode, AK_HEAP_NODE_FLAGS_SID);

  if (!sidnode)
    return NULL;

  sidCount = *(size_t *)sidnode->sid;
  off      = (uint16_t)((char *)memptr - (char *)memnode);
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

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);

  sidnode = ak_heap_ext_add(heap, heapNode, AK_HEAP_NODE_FLAGS_SID);

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
  int         itmsize;

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);

  sidnode = ak_heap_ext_add(heap, heapNode, AK_HEAP_NODE_FLAGS_SID);
  off0    = sizeof(size_t);
  off     = (uint16_t)((char *)memptr - (char *)memnode);
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
ak_sid_destroy(AkHeap * __restrict heap,
               AkSIDNode * __restrict snode) {
  AkHeapAllocator *alc;

  alc = heap->allocator;
  if (snode->refs)
    alc->free(snode->refs);

  if (snode->sids)
    alc->free(snode->sids);
}

AK_EXPORT
ptrdiff_t
ak_sidElement(AkDoc      * __restrict doc,
              const char * __restrict target,
              void      ** __restrict idnode) {
  AkHeap     *heap;
  AkHeapNode *hnode, *parent;
  const char *it;
  char       *sidp;
  AkResult    ret;

  heap = NULL;
  it   = NULL;
  sidp = NULL;

  if (*idnode) {
    hnode = ak__alignof(*idnode);
    goto again;
  }

  heap = ak_heap_getheap(doc);
  it   = ak_strltrim_fast(target);
  sidp = strchr(target, '/');

  /* if there is no "/" then assume that this is "./" */
  if (*it == '.' || !sidp) {
    AkSidConstr *constr;

    /* .[attr] */
    if (*it == '.' && !sidp) {
      it++;
      goto ret;
    }

    /* else ./sid.../sid.[attr] */

    /* find id node */
    hnode  = ak__alignof(target);

  again:
    constr = ak_sidConstraintsOf(ak_typeid((void *)target));
    parent = ak_heap_parent(hnode);

    /* find by constraint / semantic */
    if (parent && constr) {
      while (parent) {
        AkSidConstrItem *constrItem;
        AkTypeId pTypeId;
        bool     found;

        found      = false;
        pTypeId    = ak_typeid(ak__alignas(parent));
        constrItem = constr->item;
        while (constrItem) {
          if (constrItem->constr == pTypeId) {
            found = true;
            break;
          }

          constrItem = constrItem->next;
        }

        if (found)
          break;

        parent = ak_heap_parent(parent);
      }
    }

    /* check id nodes */
    else {
      while (parent) {
        void *id;

        id = ak_heap_getId(heap, parent);
        if (id)
          break;

        parent = ak_heap_parent(parent);
      }
    }

    *idnode = ak__alignas(parent);

    /* no need to offset again */
    if (*idnode)
      return 0;
  } else if (sidp) {
    char     *id;
    ptrdiff_t idlen;

    idlen = sidp - it;
    id    = malloc(sizeof(char) * idlen + 1);

    memcpy(id, it, idlen);
    id[idlen] = '\0';

    ret = ak_heap_getNodeById(heap,
                              (void *)id,
                              &hnode);
    if (ret != AK_OK || !idnode)
      goto err;

    *idnode = ak__alignas(hnode);
    it      = sidp;
  } else {
    goto err;
  }

ret:
  return it - target + 1;
err:
  return -1;
}

AK_EXPORT
void *
ak_sid_resolve(AkDoc      * __restrict doc,
               const char * __restrict target) {
  AkHeap      *heap;
  AkHeapNode  *idnode, *sidnode, *it;
  char        *siddup, *sid_it, *saveptr;
  void        *found;
  AkHeapNode **buf[2];
  void        *elm;
  size_t       bufl[2], bufc[2], bufi[2];
  int          bufidx;
  uint16_t     off;
  ptrdiff_t    sidoff;

  elm    = NULL;
  sidoff = ak_sidElement(doc, target, &elm);
  if (sidoff == -1 || !elm)
    return NULL;

  bufi[0] = bufi[1] = bufc[0] = bufc[1] = bufidx = 0;
  bufl[0] = bufl[1] = 4; /* start point */

  heap    = ak_heap_getheap(doc);
  buf[0]  = malloc(sizeof(*buf[0]) * bufl[0]);
  buf[1]  = malloc(sizeof(*buf[0]) * bufl[0]);

  do {
    idnode  = ak__alignof(elm);
    off     = 0;
    sidnode = NULL;
    found   = NULL;
    siddup  = strdup(target + sidoff);
    sid_it  = strtok_r(siddup, "/ \t", &saveptr);

    buf[bufidx][bufi[bufidx]] = ak_heap_chld(idnode);
    bufc[bufidx] = 1;

    /* breadth-first search */
    while (bufc[bufidx] > 0) {

      it = buf[bufidx][bufi[bufidx]];

      while (it) {
        if (it->flags & AK_HEAP_NODE_FLAGS_SID) {
          AkSIDNode *snode;

          snode = ak_heap_ext_get(it, AK_HEAP_NODE_FLAGS_SID);
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

    /* pick next parent*/
    (void)ak_sidElement(doc, target, &elm);
    if (!elm)
      goto err;

    free(siddup);
  } while (true);

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
  /* TODO: */
  return ak__alignas(heapNode);
}

void _assetkit_hide
ak_sid_init() {
  ak_heap_init(sidconst_heap,
               NULL,
               ak_cmp_i32,
               NULL);
  ak_sidInitConstr();
}

void _assetkit_hide
ak_sid_deinit() {
  ak_heap_destroy(sidconst_heap);
}
