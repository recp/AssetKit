/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"
#include "rb.h"
#include "../bitwise/bitwise.h"

static uint8_t ak__heap_ext_sz[] = {
  (uint8_t)sizeof(AkHeapSrchNode),
  (uint8_t)sizeof(AkSIDNode),
  (uint8_t)sizeof(int),
  (uint8_t)sizeof(uintptr_t),
  (uint8_t)sizeof(uintptr_t),
  (uint8_t)sizeof(AkUrlNode),
  (uint8_t)sizeof(uintptr_t),
  (uint8_t)sizeof(uintptr_t)
};

AK_INLINE
uint32_t
ak_heap_ext_size(uint16_t flags) {
  uint32_t sz, flag, i, ctz;

  sz   = 0;
  ctz  = (31 - ak_bitw_clz(flags));
  flag = 1 << ctz;
  i    = ctz - ak_bitw_ctz(AK_HEAP_NODE_FLAGS_EXT_FRST);

  while (flag >= AK_HEAP_NODE_FLAGS_EXT_FRST) {
    if (flags & flag)
      sz += ak__heap_ext_sz[i];
    flag >>= 1;
    i--;
  }

  return sz;
}

AK_INLINE
uint32_t
ak_heap_ext_off(uint16_t flags, uint16_t flag) {
  uint32_t sz, i;

  sz = 0;
  i = ak_bitw_ctz(flag) - ak_bitw_ctz(AK_HEAP_NODE_FLAGS_EXT_FRST);

  while ((flag >>= 1) >= AK_HEAP_NODE_FLAGS_EXT_FRST) {
    i--;
    if (flags & flag)
      sz += ak__heap_ext_sz[i];
  }

  return sz;
}

AK_INLINE
void
ak_heap_ext_freeurl(AkHeapNode * __restrict hnode) {
  AkUrlNode  *urlNode;
  void       *urlobj;
  void       **found, **it, *last;
  size_t      len;

  urlNode = ak_heap_ext_get(hnode, AK_HEAP_NODE_FLAGS_REFC);
  len     = urlNode->len;
  it      = urlNode->urls[0];
  last    = urlNode->urls[len];
  found   = NULL;

  while (it != last) {
    /* check if object is available */
    if ((urlobj = ak_getObjectByUrl(*it)))
      ak_release(urlobj);
    it++;
  }
}

void *
ak_heap_ext_get(AkHeapNode * __restrict hnode,
                uint16_t                flag) {
  AkHeapNodeExt *exnode;
  uint32_t       ofst;

  if (!(hnode->flags & flag))
    return NULL;

  exnode = hnode->chld;
  ofst   = ak_heap_ext_off(hnode->flags & ~flag, flag);

  return &exnode->data[ofst];
}

void *
ak_heap_ext_add(AkHeap     * __restrict heap,
                AkHeapNode * __restrict hnode,
                uint16_t                flag) {
  AkHeapAllocator *alc;
  AkHeapNodeExt   *exnode;
  uint32_t         sz, ofst, isz, flag_off;

  ofst = ak_heap_ext_off(hnode->flags, flag);

  /* nothing to do */
  if (hnode->flags & flag) {
    exnode = hnode->chld;
    return &exnode->data[ofst];
  }

  flag_off = ak_bitw_ctz(AK_HEAP_NODE_FLAGS_EXT_FRST);

  alc = heap->allocator;
  sz  = ak_heap_ext_size(hnode->flags | flag);
  isz = ak__heap_ext_sz[ak_bitw_ctz(flag >> flag_off)];

  if (!(hnode->flags & AK_HEAP_NODE_FLAGS_EXT)) {
    exnode        = alc->malloc(sizeof(*exnode) + sz);
    exnode->node  = hnode;
    exnode->chld  = hnode->chld;
    hnode->flags |= AK_HEAP_NODE_FLAGS_EXT;
  } else {
    AkHeapSrchNode *curr, *parent;
    int side, tomove;

    exnode = hnode->chld;
    parent = NULL;
    side   = 1;

    /* save link */
    if (hnode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
      curr = (AkHeapSrchNode *)exnode->data;
      side = ak_heap_rb_parent(heap->srchctx, curr->key, &parent);
    }

    exnode = alc->realloc(exnode, sizeof(*exnode) + sz);
    tomove = sz - isz - ofst;
    
    if (tomove > 0)
      memmove(&exnode->data[ofst + isz],
              &exnode->data[ofst],
              tomove);

    /* update link */
    if (parent) {
      curr = (AkHeapSrchNode *)exnode->data;
      parent->chld[side] = curr;
    }
  }

  memset(&exnode->data[ofst], 0, isz);
  hnode->chld = exnode;

  hnode->flags |= flag;

  return &exnode->data[ofst];
}

void
ak_heap_ext_rm(AkHeap     * __restrict heap,
               AkHeapNode * __restrict hnode,
               uint16_t                flag) {
  AkHeapAllocator *alc;
  AkHeapNodeExt   *exnode;
  uint32_t         sz, ofst, isz, flag_off;

  /* nothing to do */
  if (!(hnode->flags & flag))
    return;

  flag_off = ak_bitw_ctz(AK_HEAP_NODE_FLAGS_EXT_FRST);

  alc    = heap->allocator;
  sz     = ak_heap_ext_size(hnode->flags & ~flag);
  ofst   = ak_heap_ext_off(hnode->flags, flag);
  isz    = ak__heap_ext_sz[ak_bitw_ctz(flag >> flag_off)];
  exnode = hnode->chld;

  /* free items */
  switch (flag) {
    case AK_HEAP_NODE_FLAGS_SRCH:
      ak_heap_rb_remove(heap->srchctx,
                        (AkHeapSrchNode *)&exnode->data[ofst]);
      hnode->flags &= ~AK_HEAP_NODE_FLAGS_RED;
      break;
    case AK_HEAP_NODE_FLAGS_SID:
      ak_sid_destroy(heap, (AkSIDNode *)&exnode->data[ofst]);
      break;
    case AK_HEAP_NODE_FLAGS_EXTRA:
    case AK_HEAP_NODE_FLAGS_INF:
      /* ak_free(&exnode->data[ofst]); */
      break;
    case AK_HEAP_NODE_FLAGS_USR:
      if (hnode->flags & AK_HEAP_NODE_FLAGS_USRF)
        alc->free(&exnode->data[ofst]);
      break;
    case AK_HEAP_NODE_FLAGS_URL:
      ak_heap_ext_freeurl(hnode);
      break;
  }

  hnode->flags &= ~flag;
  if (sz > 0) {
    AkHeapSrchNode *curr, *parent;
    int side;

    parent = NULL;
    side   = 1;

    /* save link */
    if (hnode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
      curr = (AkHeapSrchNode *)exnode->data;
      side = ak_heap_rb_parent(heap->srchctx,
                               curr->key,
                               &parent);
    }

    if (sz > ofst)
      memmove(&exnode->data[ofst] + isz,
              &exnode->data[ofst],
              sz - ofst);

    exnode = alc->realloc(exnode, sz + sizeof(*exnode));

    /* update link */
    if (parent) {
      curr = (AkHeapSrchNode *)exnode->data;
      parent->chld[side] = curr;
    }

    hnode->chld = exnode;
  } else {
    hnode->chld = exnode->chld;
    alc->free(exnode);

    hnode->flags &= ~AK_HEAP_NODE_FLAGS_EXT_ALL;
  }
}

void
ak_heap_ext_free(AkHeap     * __restrict heap,
                 AkHeapNode * __restrict hnode) {
  AkHeapAllocator *alc;
  AkHeapNodeExt   *exnode;
  uint32_t         ofst;

  /* nothing to do */
  if (!(hnode->flags & AK_HEAP_NODE_FLAGS_EXT))
    return;

  alc    = heap->allocator;
  exnode = hnode->chld;

  /* free items */
  if (hnode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    ofst = ak_heap_ext_off(hnode->flags, AK_HEAP_NODE_FLAGS_SRCH);
    ak_heap_rb_remove(heap->srchctx,
                      (AkHeapSrchNode *)&exnode->data[ofst]);
  }

  if (hnode->flags & AK_HEAP_NODE_FLAGS_SID) {
    ofst = ak_heap_ext_off(hnode->flags, AK_HEAP_NODE_FLAGS_SID);
    ak_sid_destroy(heap, (AkSIDNode *)&exnode->data[ofst]);
  }

  if (hnode->flags & AK_HEAP_NODE_FLAGS_EXTRA) {
    /* ofst = ak_heap_ext_off(hnode->flags, AK_HEAP_NODE_FLAGS_EXTRA);
       ak_free(&exnode->data[ofst]); */
  }

  if (hnode->flags & AK_HEAP_NODE_FLAGS_INF) {
    /* ofst = ak_heap_ext_off(hnode->flags, AK_HEAP_NODE_FLAGS_INF);
       ak_free(&exnode->data[ofst]); */
  }

  if (hnode->flags & AK_HEAP_NODE_FLAGS_URL) {
    ak_heap_ext_freeurl(hnode);
  }

  if (hnode->flags & AK_HEAP_NODE_FLAGS_USR) {
    ofst = ak_heap_ext_off(hnode->flags, AK_HEAP_NODE_FLAGS_INF);
    if (hnode->flags & AK_HEAP_NODE_FLAGS_USRF)
      alc->free(&exnode->data[ofst]);
  }

  hnode->chld = exnode->chld;
  alc->free(exnode);

  hnode->flags &= ~AK_HEAP_NODE_FLAGS_EXT_ALL;
}
