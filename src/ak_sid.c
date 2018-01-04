/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "../include/ak-profile.h"
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
  uintptr_t   sidptrval;
  int         off0;
  int         itmsize;

  heapNode = ak__alignof(memnode);
  heap     = ak_heap_getheap(memnode);

  sidnode = ak_heap_ext_add(heap, heapNode, AK_HEAP_NODE_FLAGS_SID);
  off0    = sizeof(size_t);
  off     = (uint16_t)((char *)memptr - (char *)memnode);
  itmsize = sizeof(uint16_t) + sizeof(uintptr_t);

  if (!sidnode->sids) {
    sidnode->sids = heap->allocator->calloc(1, off0 + itmsize);
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

  sidptr   += sizeof(uint16_t);
  sidptrval = (uintptr_t)sid;
  memcpy(sidptr, &sidptrval, sizeof(uintptr_t));

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

_assetkit_hide
ptrdiff_t
ak_sidElement(AkContext  * __restrict ctx,
              const char * __restrict target,
              void      ** __restrict idnode,
              bool       * __restrict isdot) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkHeapNode *hnode, *parent;
  const char *it;
  char       *sidp;
  AkResult    ret;

  heap  = NULL;
  it    = ak_strltrim_fast(target);
  sidp  = strchr(target, '/');
  *isdot = *it == '.' || !sidp;

  if (*idnode && *isdot) {
    hnode = ak__alignof(*idnode);
    goto again;
  }

  doc  = ctx->doc;
  heap = ak_heap_getheap(doc);


  /* if there is no "/" then assume that this is "./" */
  if (*isdot) {
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
    free(id);
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

_assetkit_hide
AkHeapNode*
ak_sid_profile(AkContext  * __restrict ctx,
               AkHeapNode * __restrict parent,
               AkHeapNode * __restrict after) {
  AkProfile       *profile;
  AkTechniqueHint *hint;
  const char      *platform;

  profile = ak_profile(ak__alignas(parent),
                       after ? ak__alignas(after) : NULL);

  /* check hint for profile */
  if ((!ctx->techniqueHint
       || !ctx->techniqueHint->profile)
      && profile)
    return ak__alignof(profile);

  hint     = ctx->techniqueHint;
  platform = ak_platform();
  while (profile) {
    if (profile->type == hint->profileType)
      goto ret;

    if (!(profile = ak_profile(ak__alignas(parent), profile)))
      goto err;

    /* check platform */
    if (hint->platform) {
      if (strcmp(platform, hint->platform) == 0)
        profile = ak_profile(ak__alignas(parent), profile);
    }
  }

ret:
  if (profile)
    return ak__alignof(profile);

err:
  return NULL;
}

_assetkit_hide
AkHeapNode*
ak_sid_technique(AkContext  * __restrict ctx,
                 AkHeapNode * __restrict chld) {
  AkHeapNode *orig;
  orig = chld;

  /* first check hint for technique */
  if (ctx->techniqueHint && ctx->techniqueHint->ref) {
    AkTechniqueHint *hint;
    const char      *platform;

    hint     = ctx->techniqueHint;
    platform = ak_platform();

    /* get desired technique */
    while (chld) {
      AkSIDNode *snode;
      snode = ak_heap_ext_get(chld, AK_HEAP_NODE_FLAGS_SID);
      if (snode->sid && strcmp(snode->sid, hint->ref) == 0)
        goto ret;

      if (!(chld = chld->next))
        goto err;

      /* check platform */
      if (hint->platform) {
        if (strcmp(platform, hint->platform) == 0)
          chld = chld->next;
      }
    }
  }

  /* check global options to get active technique name */
  else {
    /* default: try to get sid="common" or technique_common */
    char **tnames, *tname;

    /* get global techniques with order */
    if (ak_typeidh(chld) == AKT_TECHNIQUE_FX)
      tnames = (char **)ak_opt_get(AK_OPT_TECHNIQUE_FX);
    else
      tnames = (char **)ak_opt_get(AK_OPT_TECHNIQUE);

    if (!*tnames)
      goto err;

    /* get desired technique */
    while (chld) {
      tname = *tnames;

      do {
        AkSIDNode *snode;
        uint32_t   i;

        i     = 0;
        snode = ak_heap_ext_get(chld, AK_HEAP_NODE_FLAGS_SID);
        if (snode
            && snode->sid
            && strcmp(snode->sid, tname) == 0)
          goto ret;

        tname = tnames[++i];
      } while (tname);

      chld = chld->next;
    }
  }

  /* default: no technique found use first one */
  chld = orig;

ret:
  return chld;

err:
  return NULL;
}

_assetkit_hide
AkHeapNode*
ak_sid_chldh(AkContext  * __restrict ctx,
             AkHeapNode * __restrict parent,
             AkHeapNode * __restrict after) {
  AkHeapNode *chld;

  /* select active profile if parent is <effect> */
  if (ak_typeidh(parent) == AKT_EFFECT)
    return ak_sid_profile(ctx, parent, after);

  /* get child element */
  if (after)
    chld = after->next;
  else
    chld = ak_heap_chld(parent);

  if (!chld)
    return NULL;

  /* check active technique if child element is <technique> */
  if (ak_typeidh(chld) == AKT_TECHNIQUE_FX
      || ak_typeidh(chld) == AKT_TECHNIQUE)
    chld = ak_sid_technique(ctx, chld);

  return chld;
}

AK_EXPORT
void *
ak_sid_resolve(AkContext  * __restrict ctx,
               const char * __restrict target) {
  AkHeapNode  *idnode, *sidnode, *it, *chld;
  char        *siddup, *sid_it, *saveptr;
  void        *found;
  AkHeapNode **buf[2];
  void        *elm;
  size_t       bufl[2], bufc[2], bufi[2];
  int          bufidx;
  uint16_t     off;
  ptrdiff_t    sidoff;
  bool         isdot;

  elm    = NULL;
  sidoff = ak_sidElement(ctx, target, &elm, &isdot);
  if (sidoff == -1 || !elm)
    return NULL;

  bufi[0] = bufi[1] = bufc[0] = bufc[1] = bufidx = 0;
  bufl[0] = bufl[1] = 4; /* start point */

  buf[0]  = malloc(sizeof(*buf[0]) * bufl[0]);
  buf[1]  = malloc(sizeof(*buf[0]) * bufl[0]);

again:
  idnode  = ak__alignof(elm);
  off     = 0;
  sidnode = NULL;
  found   = NULL;
  siddup  = strdup(target + sidoff);
  sid_it  = strtok_r(siddup, "/ \t", &saveptr);

  chld = ak_sid_chldh(ctx, idnode, NULL);
  if (!chld)
    return NULL;

  buf[bufidx][bufi[bufidx]] = chld;
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

        it2 = ak_sid_chldh(ctx, it, NULL);
        while (it2) {
          if (bufi[!bufidx] == bufl[!bufidx]) {
            bufl[!bufidx] += 16;
            buf[!bufidx]   = realloc(buf[!bufidx],
                                     sizeof(*buf[!bufidx]) * bufl[!bufidx]);
          }

          buf[!bufidx][bufi[!bufidx]] = it2;
          bufi[!bufidx]++;
          bufc[!bufidx]++;

          it2 = ak_sid_chldh(ctx, it, it2);
        }
      }

      it = it->next;
    }

    if (++bufi[bufidx] == bufc[bufidx]) {
      bufc[bufidx] = bufi[bufidx] = bufi[!bufidx] = 0;
      bufidx       = !bufidx;
    }
  }

  if (isdot) {
    /* pick next parent*/
    (void)ak_sidElement(ctx, target, &elm, &isdot);
    if (!elm)
      goto err;

    free(siddup);
    goto again;
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
