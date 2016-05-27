/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_memory_redblack_h
#define ak_memory_redblack_h

#include "ak_memory_common.h"

#define AK__RB_ISRED(X)  (AK__HEAPNODE(X)->flags & AK_HEAP_NODE_FLAGS_RED)
#define AK__RB_MKRED(X)   AK__HEAPNODE(X)->flags |= AK_HEAP_NODE_FLAGS_RED
#define AK__RB_MKBLACK(X) AK__HEAPNODE(X)->flags &= ~AK_HEAP_NODE_FLAGS_RED

void
ak_heap_rb_insert(AkHeapSrchContext * __restrict srchCtx,
                  AkHeapSrchNode * __restrict srchNode);

void
ak_heap_rb_remove(AkHeapSrchContext * __restrict srchCtx,
                  AkHeapSrchNode * __restrict srchNode);

AkHeapSrchNode *
ak_heap_rb_find(AkHeapSrchContext * __restrict srchCtx,
                void * __restrict key);

void
ak_heap_rb_print(AkHeapSrchContext * __restrict srchCtx);

#endif /* ak_memory_redblack_h */
