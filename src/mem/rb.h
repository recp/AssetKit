/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ak_memory_redblack_h
#define ak_memory_redblack_h

#include "common.h"

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

int
ak_heap_rb_assert(AkHeapSrchCtx * srchctx,
                  AkHeapSrchNode * root);

#endif /* ak_memory_redblack_h */
