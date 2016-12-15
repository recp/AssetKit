/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "ak_memory_rb.h"
#include "bitwise/ak_bitwise.h"

static uint8_t ak__heap_ext_sz[] = {
  (uint8_t)sizeof(AkHeapSrchNode),
  (uint8_t)sizeof(AkSIDNode),
  (uint8_t)sizeof(uintptr_t),
  (uint8_t)sizeof(uintptr_t),
  (uint8_t)sizeof(uintptr_t),
  (uint8_t)sizeof(uintptr_t)
};

static
AK_INLINE
uint32_t
ak_heap_ext_size(uint16_t flags) {
  uint32_t sz, flag, i;

  sz   = 0;
  flag = (1 << (31 - ak_bitw_clz(flags))) >> AK_HEAP_NODE_FLAGS_EXT_FRST;
  i    = ak_bitw_ctz(flag);

  while (flag > 0) {
    if (flags & flag)
      sz += ak__heap_ext_sz[i--];
    flag >>= 1;
  }

  return sz;
}

static
AK_INLINE
uint32_t
ak_heap_ext_off(uint16_t flags, uint16_t flag) {
  uint32_t sz, i;

  flag >>= AK_HEAP_NODE_FLAGS_EXT_FRST;
  sz = 0;
  i = ak_bitw_ctz(flag);

  while (flag > 0) {
    if (flags & flag)
      sz += ak__heap_ext_sz[i--];
    flag >>= 1;
  }

  return sz;
}

void *
ak_heap_ext_get(AkHeapNode * __restrict hnode,
                uint16_t                flag) {
  AkHeapNodeExt   *exnode;
  uint32_t         ofst;

  ofst = ak_heap_ext_off(hnode->flags, flag);

  /* nothing to do */
  if (hnode->flags & flag) {
    exnode = hnode->chld;
    return &exnode->data[ofst];
  }

  return NULL;
}

void *
ak_heap_ext_add(AkHeap     * __restrict heap,
                AkHeapNode * __restrict hnode,
                uint16_t                flag) {
  AkHeapAllocator *alc;
  AkHeapNodeExt   *exnode;
  uint32_t         sz, ofst, isz;

  ofst = ak_heap_ext_off(hnode->flags, flag);

  /* nothing to do */
  if (hnode->flags & flag) {
    exnode = hnode->chld;
    return &exnode->data[ofst];
  }

  alc = heap->allocator;
  sz  = ak_heap_ext_size(hnode->flags);
  isz = ak__heap_ext_sz[ak_bitw_ctz(flag >> AK_HEAP_NODE_FLAGS_EXT_FRST)];

  if (!(hnode->flags & AK_HEAP_NODE_FLAGS_EXT)) {
    exnode        = alc->malloc(sizeof(*exnode) + sz + isz);
    exnode->node  = hnode;
    exnode->chld  = hnode->chld;
    hnode->flags |= AK_HEAP_NODE_FLAGS_EXT;
  } else {
    exnode = alc->realloc(hnode->chld, sizeof(*exnode) + sz + isz);
    memmove(&exnode->data[ofst],
            &exnode->data[ofst] + isz,
            isz);
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
  uint32_t         sz, ofst, isz;

  /* nothing to do */
  if (!(hnode->flags & flag))
    return;

  alc    = heap->allocator;
  sz     = ak_heap_ext_size(hnode->flags & ~flag);
  ofst   = ak_heap_ext_off(hnode->flags, flag);
  isz    = ak__heap_ext_sz[ak_bitw_ctz(flag >> AK_HEAP_NODE_FLAGS_EXT_FRST)];
  exnode = hnode->chld;

  /* free items */
  switch (flag) {
    case AK_HEAP_NODE_FLAGS_SRCH:
      ak_heap_rb_remove(heap->srchctx,
                        (AkHeapSrchNode *)&exnode->data[ofst]);
      hnode->flags &= ~AK_HEAP_NODE_FLAGS_RED;
      break;
    case AK_HEAP_NODE_FLAGS_SID:
      ak_sid_destroy((AkSIDNode *)&exnode->data[ofst]);
      break;
    case AK_HEAP_NODE_FLAGS_EXTRA:
    case AK_HEAP_NODE_FLAGS_INF:
      ak_free(&exnode->data[ofst]);
      break;
    case AK_HEAP_NODE_FLAGS_USR:
      if (hnode->flags & AK_HEAP_NODE_FLAGS_USRF)
        alc->free(&exnode->data[ofst]);
      break;
  }

  hnode->flags &= ~flag;
  if (sz > 0) {
    exnode = alc->realloc(hnode->chld, sz + sizeof(*exnode));
    memmove(&exnode->data[ofst] + isz,
            &exnode->data[ofst],
            isz);
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
    ak_sid_destroy((AkSIDNode *)&exnode->data[ofst]);
  }

  if (hnode->flags & AK_HEAP_NODE_FLAGS_EXTRA) {
    ofst = ak_heap_ext_off(hnode->flags, AK_HEAP_NODE_FLAGS_EXTRA);
    ak_free(&exnode->data[ofst]);
  }

  if (hnode->flags & AK_HEAP_NODE_FLAGS_INF) {
    ofst = ak_heap_ext_off(hnode->flags, AK_HEAP_NODE_FLAGS_INF);
    ak_free(&exnode->data[ofst]);
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
