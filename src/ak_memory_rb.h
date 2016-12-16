/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_memory_redblack_h
#define ak_memory_redblack_h

#include "ak_memory_common.h"

#define AK__RB_ISBLACK(X) !((AK__HEAPNODE(X)->flags & AK_HEAP_NODE_FLAGS_RED))
#define AK__RB_ISRED(X)     (AK__HEAPNODE(X)->flags & AK_HEAP_NODE_FLAGS_RED)
#define AK__RB_MKRED(X)    AK__HEAPNODE(X)->flags |= AK_HEAP_NODE_FLAGS_RED
#define AK__RB_MKBLACK(X)  AK__HEAPNODE(X)->flags &= ~AK_HEAP_NODE_FLAGS_RED

void
ak_heap_rb_insert(AkHeapSrchCtx * __restrict srchctx,
                  AkHeapSrchNode * __restrict srchNode);

void
ak_heap_rb_remove(AkHeapSrchCtx * __restrict srchctx,
                  AkHeapSrchNode * __restrict srchNode);

AkHeapSrchNode *
ak_heap_rb_find(AkHeapSrchCtx * __restrict srchctx,
                void * __restrict key);

int
ak_heap_rb_parent(AkHeapSrchCtx * __restrict srchctx,
                  void * __restrict key,
                  AkHeapSrchNode ** dest);

void
ak_heap_rb_print(AkHeapSrchCtx * __restrict srchctx);

#endif /* ak_memory_redblack_h */
