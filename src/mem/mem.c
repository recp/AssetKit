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

#include "common.h"
#include "rb.h"
#include "lt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static
int
ak__heap_srch_cmp(void * __restrict key1,
                  void * __restrict key2);

static
void
ak__heap_srch_print(void * __restrict key);

static
char*
ak__heap_strdup_def(const char * str);

_assetkit_hide
void
ak_heap_moveh_chld(AkHeap     * __restrict heap,
                   AkHeap     * __restrict newheap,
                   AkHeapNode * __restrict heapNode);

static void * ak__emptystr = "";

AkHeapAllocator ak__allocator = {
  .malloc   = malloc,
  .calloc   = calloc,
  .realloc  = realloc,
  .free     = free,
#ifndef _WIN32
  .memalign = posix_memalign,
#endif
  .strdup   = ak__heap_strdup_def
};

static AkHeap ak__heap = {
  .allocator  = &ak__allocator,
  .srchctx    = NULL,
  .root       = NULL,
  .trash      = NULL,
  .flags      = 0
};

static RBTree *ak__heap_sub = NULL;

static
int
ak__heap_srch_cmp(void * __restrict key1,
                  void * __restrict key2) {
  return strcmp((char *)key1, (char *)key2);
}

static
void
ak__heap_srch_print(void * __restrict key) {
  printf("\t'%s'\n", (const char *)key);
}

static
char*
ak__heap_strdup_def(const char * str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak__heap.allocator->malloc(memsize + 1);
  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

AK_EXPORT
char*
ak_heap_strdup(AkHeap * __restrict heap,
               void   * __restrict parent,
               const char * str) {
  void  *memptr;
  size_t memsize;

  if (!str)
    return NULL;

  memsize = strlen(str);
  memptr  = ak_heap_alloc(heap, parent, memsize + 1);

  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

AK_EXPORT
char*
ak_heap_strndup(AkHeap     * __restrict heap,
                void       * __restrict parent,
                const char *            str,
                size_t size) {
  void *memptr;

  memptr  = ak_heap_alloc(heap, parent, size + 1);
  memcpy(memptr, str, size);

  /* NULL */
  memset((char *)memptr + size, '\0', 1);

  return memptr;
}

AK_EXPORT
AkHeapAllocator *
ak_heap_allocator(AkHeap * __restrict heap) {
  return heap->allocator;
}

AK_EXPORT
AkHeap *
ak_heap_default() {
  return &ak__heap;
}

AK_EXPORT
AkHeap *
ak_heap_getheap(void * __restrict memptr) {
  AkHeapNode *heapNode;
  heapNode = ak__alignof(memptr);
  return ak_heap_lt_find(heapNode->heapid);
}

AK_EXPORT
AkHeap *
ak_heap_new(AkHeapAllocator  *allocator,
            AkHeapSrchCmpFn   cmp,
            AkHeapSrchPrintFn print) {
  AkHeapAllocator *alc;
  AkHeap          *heap;

  alc = allocator ? allocator : &ak__allocator;

  heap = alc->malloc(sizeof(*heap));
  assert(heap && "malloc failed");

  heap->flags = AK_HEAP_FLAGS_DYNAMIC;
  ak_heap_init(heap, allocator, cmp, print);

  return heap;
}

AK_EXPORT
void
ak_heap_attach(AkHeap * __restrict parent,
               AkHeap * __restrict chld) {
  chld->next   = parent->chld;
  parent->chld = chld;
}

AK_EXPORT
void
ak_heap_dettach(AkHeap * __restrict parent,
                AkHeap * __restrict chld) {
  AkHeap *heap;

  heap = parent->chld;
  if (heap == chld) {
    parent->chld = chld->next;
    chld->next   = NULL;
    return;
  }

  while (heap) {
    if (heap->next == chld) {
      heap->next = chld->next;
      chld->next = NULL;
      break;
    }

    heap = heap->next;
  }
}

AK_EXPORT
void
ak_heap_setdata(AkHeap * __restrict heap,
                void * __restrict memptr) {
  heap->data = memptr;
}

AK_EXPORT
void*
ak_heap_data(AkHeap * __restrict heap) {
  return heap->data;
}

AK_EXPORT
void
ak_heap_init(AkHeap          * __restrict heap,
             AkHeapAllocator * __restrict allocator,
             AkHeapSrchCmpFn              cmp,
             AkHeapSrchPrintFn            print) {
  AkHeapAllocator *alc;
  AkHeapSrchCtx   *srchctx;
  AkHeapNode      *rootNode,     *nullNode;
  AkHeapNodeExt   *rootNodeExt,  *nullNodeExt;
  AkHeapSrchNode  *rootSrchNode, *nullSrchNode;

  if (heap->flags & AK_HEAP_FLAGS_INITIALIZED)
    return;

  alc = allocator ? allocator : &ak__allocator;

  srchctx     = alc->malloc(sizeof(*srchctx));
  rootNode    = alc->calloc(1, ak__heapnd_sz);
  nullNode    = alc->calloc(1, ak__heapnd_sz);
  rootNodeExt = alc->calloc(1, sizeof(*rootNodeExt) + sizeof(AkHeapSrchNode));
  nullNodeExt = alc->calloc(1, sizeof(*nullNodeExt) + sizeof(AkHeapSrchNode));

  assert(srchctx
         && rootNode
         && nullNode
         && rootNodeExt
         && nullNodeExt
         && "malloc failed");

  rootNode->chld    = rootNodeExt;
  rootNodeExt->node = rootNode;
  rootSrchNode      = (AkHeapSrchNode *)rootNodeExt->data;

  nullNode->chld    = nullNodeExt;
  nullNodeExt->node = nullNode;
  nullSrchNode      = (AkHeapSrchNode *)nullNodeExt->data;

  nullSrchNode->key = ak__emptystr;
  nullSrchNode->chld[AK__BST_LEFT]  = nullSrchNode;
  nullSrchNode->chld[AK__BST_RIGHT] = nullSrchNode;

  rootSrchNode->key = ak__emptystr;
  rootSrchNode->chld[AK__BST_LEFT]  = nullSrchNode;
  rootSrchNode->chld[AK__BST_RIGHT] = nullSrchNode;

  rootNode->flags   = (AK_HEAP_NODE_FLAGS_EXT | AK_HEAP_NODE_FLAGS_SRCH);
  nullNode->flags   = (AK_HEAP_NODE_FLAGS_EXT | AK_HEAP_NODE_FLAGS_SRCH);

  srchctx->cmp      = cmp   ? cmp   : ak__heap_srch_cmp;
  srchctx->print    = print ? print : ak__heap_srch_print;
  srchctx->root     = rootSrchNode;
  srchctx->nullNode = nullSrchNode;

  heap->chld      = NULL;
  heap->next      = NULL;
  heap->root      = NULL;
  heap->trash     = NULL;
  heap->data      = NULL;
  heap->idheap    = NULL;
  heap->heapid    = 0;
  heap->allocator = alc;
  heap->srchctx   = srchctx;
  heap->flags    |= AK_HEAP_FLAGS_INITIALIZED;

  if (heap != &ak__heap)
    ak_heap_lt_insert(heap);
}

AK_EXPORT
void
ak_heap_destroy(AkHeap * __restrict heap) {
  AkHeapAllocator *alc;
  AkHeapNode      *rootNode, *nullNode;
  AkHeap          *it, *toDestroy;

  if (!(heap->flags & AK_HEAP_FLAGS_INITIALIZED))
    return;

  alc = heap->allocator;

  /* first destroy all attached heaps */
  if (heap->chld) {
    it = heap->chld;
    do {
      toDestroy = it;
      it = it->next;
      ak_heap_destroy(toDestroy);
    } while (it);
  }

  ak_heap_cleanup(heap);

  rootNode = AK__HEAPNODE(heap->srchctx->root);
  nullNode = AK__HEAPNODE(heap->srchctx->nullNode);

  alc->free(rootNode->chld);
  alc->free(nullNode->chld);
  alc->free(rootNode);
  alc->free(nullNode);
  alc->free(heap->srchctx);

  heap->data = NULL;
  ak_heap_lt_remove(heap->heapid);

  if (heap->flags & AK_HEAP_FLAGS_DYNAMIC
      && heap != &ak__heap)
    alc->free(heap);
}

AK_EXPORT
void*
ak_heap_alloc(AkHeap * __restrict heap,
              void   * __restrict parent,
              size_t              size) {
  AkHeapNode *currNode;
  AkHeapNode *parentNode;
  size_t      memsize;

  assert((!parent || heap->heapid == ak__alignof(parent)->heapid)
         && "parent and child mem must use same heap");

  memsize  = ak__heapnd_sz + size;
  memsize  = ak__align(memsize);
  currNode = heap->allocator->malloc(memsize);
  assert(currNode && "malloc failed");

  currNode->flags  = 0;
  currNode->typeid = 0;
  currNode->chld   = NULL;
  currNode->heapid = heap->heapid;

  if (parent) {
    AkHeapNode *chldNode;

    parentNode = ak__alignof(parent);
    chldNode   = ak_heap_chld(parentNode);

    ak_heap_chld_set(parentNode, currNode);
    if (chldNode)
      chldNode->prev = currNode;

    currNode->next = chldNode;
  } else {
    if (heap->root)
      heap->root->prev = currNode;

    currNode->next = heap->root;
    currNode->prev = NULL;
    heap->root     = currNode;
  }

  return ak__alignas(currNode);
}

AK_EXPORT
void*
ak_heap_calloc(AkHeap * __restrict heap,
               void   * __restrict parent,
               size_t              size) {
  void *memptr;

  memptr = ak_heap_alloc(heap,
                         parent,
                         size);
  memset(memptr, '\0', size);

  return memptr;
}

AK_EXPORT
void*
ak_heap_realloc(AkHeap * __restrict heap,
                void   * __restrict parent,
                void   * __restrict memptr,
                size_t              newsize) {
  AkHeapNode *oldNode;
  AkHeapNode *newNode;
  AkHeapNode *chld;

  if (!memptr)
    return ak_heap_alloc(heap,
                         parent,
                         newsize);

  oldNode = ak__alignof(memptr);

  if (newsize == 0) {
    ak_heap_free(heap, oldNode);
    return NULL;
  }

  newsize = ak__heapnd_sz + newsize;
  newsize = ak__align(newsize);
  newNode = heap->allocator->realloc(oldNode, newsize);
  assert(newNode && "realloc failed");

  if (heap->root == oldNode)
    heap->root = newNode;

  if (heap->trash == oldNode)
    heap->trash = newNode;

  chld = newNode->chld;
  if (chld) {
    if (newNode->flags & AK_HEAP_NODE_FLAGS_EXT) {
      AkHeapNodeExt *exnode;
      exnode       = newNode->chld;
      exnode->node = newNode;

      if (exnode->chld)
        exnode->chld->prev = newNode;
    } else {
      chld->prev = newNode;
    }
  }

  if (newNode->next)
    newNode->next->prev = newNode;

  if (newNode->prev) {
    chld = ak_heap_chld(newNode->prev);

    if (chld == oldNode)
      ak_heap_chld_set(newNode->prev, newNode);
    else
      newNode->prev->next = newNode;
  }

  return ak__alignas(newNode);
}

void *
ak_heap_chld(AkHeapNode *heapNode) {
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
    return ((AkHeapNodeExt *)heapNode->chld)->chld;

  return heapNode->chld;
}

void
ak_heap_chld_set(AkHeapNode * __restrict heapNode,
                 AkHeapNode * __restrict chldNode) {
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
    ((AkHeapNodeExt *)heapNode->chld)->chld = chldNode;
  else
    heapNode->chld = chldNode;

  if (chldNode)
    chldNode->prev = heapNode;
}

AK_EXPORT
AkHeapNode *
ak_heap_parent(AkHeapNode *heapNode) {
  AkHeapNode *p, *it;

  p  = NULL;
  it = heapNode;

  while (it->prev) {
    /* we found near parent */
    if (ak_heap_chld(it->prev) == it) {
      p = it->prev;
      break;
    }

    it = it->prev;
  }

  return p;
}

AK_EXPORT
void
ak_heap_setp(AkHeapNode * __restrict heapNode,
             AkHeapNode * __restrict newParent) {
  AkHeap *oldheap, *newheap;

  oldheap = ak_heap_lt_find(heapNode->heapid);
  newheap = ak_heap_lt_find(newParent->heapid);

  if (heapNode->prev) {
    if (ak_heap_chld(heapNode->prev) == heapNode)
      ak_heap_chld_set(heapNode->prev, heapNode->next);
    else
      heapNode->prev->next = heapNode->next;

    if (heapNode->next)
      heapNode->next->prev = heapNode->prev;

    heapNode->prev = NULL;
  } else if (heapNode == oldheap->root) { /* root->prev = NULL */
    oldheap->root = heapNode->next;

    if (heapNode->next)
      heapNode->next->prev = NULL;
  }

  heapNode->next = NULL;

  if (heapNode == oldheap->trash)
    oldheap->trash = heapNode->next;

  /* move all ids to new heap (if it is different) */
  if (newParent->heapid != heapNode->heapid)
    ak_heap_moveh(heapNode, newheap);

  heapNode->next = ak_heap_chld(newParent);
  ak_heap_chld_set(newParent, heapNode);

  if (heapNode->next)
    heapNode->next->prev = heapNode;
}

_assetkit_hide
void
ak_heap_moveh_chld(AkHeap     * __restrict heap,
                   AkHeap     * __restrict newheap,
                   AkHeapNode * __restrict heapNode) {
  do {
    AkHeapNode *chld;

    if (heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
      AkHeapSrchNode *srchNode;
      srchNode = (AkHeapSrchNode *)((AkHeapNodeExt *)heapNode->chld)->data;

      ak_heap_rb_remove(heap->srchctx, srchNode);
      ak_heap_rb_insert(newheap->srchctx, srchNode);
    }

    heapNode->heapid = newheap->heapid;
    chld = ak_heap_chld(heapNode);
    if (chld)
      ak_heap_moveh_chld(heap, newheap, chld);

    heapNode = heapNode->next;
  } while (heapNode);
}

AK_EXPORT
void
ak_heap_moveh(AkHeapNode * __restrict heapNode,
              AkHeap     * __restrict newheap) {
  AkHeapNode *chld;
  AkHeap     *heap;

  heap = ak_heap_lt_find(heapNode->heapid);
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    AkHeapSrchNode *srchNode;
    srchNode = (AkHeapSrchNode *)((AkHeapNodeExt *)heapNode->chld)->data;

    ak_heap_rb_remove(heap->srchctx, srchNode);
    ak_heap_rb_insert(newheap->srchctx, srchNode);
  }

  heapNode->heapid = newheap->heapid;
  chld = ak_heap_chld(heapNode);
  if (chld)
    ak_heap_moveh_chld(heap, newheap, chld);
}

AK_EXPORT
void
ak_heap_setpm(void * __restrict memptr,
              void * __restrict newparent) {
  ak_heap_setp(ak__alignof(memptr),
               ak__alignof(newparent));
}

void _assetkit_hide
ak_freeh(AkHeapNode * __restrict heapNode) {
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_HEAP_CHLD) {
    AkHeap *attachedHeap;
    attachedHeap = ak_attachedHeap(ak__alignas(heapNode));
    if (attachedHeap) {
      ak_heap_destroy(attachedHeap);
      ak_setAttachedHeap(ak__alignas(heapNode), NULL);
    }
  }
}

AK_EXPORT
void
ak_heap_free(AkHeap     * __restrict heap,
             AkHeapNode * __restrict heapNode) {
  AkHeapAllocator * __restrict alc;
  alc = heap->allocator;

  if (heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
    ak_heap_ext_free(heap, heapNode);

  /* free attached heap */
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_HEAP_CHLD)
    ak_freeh(heapNode);

  /* free all child nodes */
  if (heapNode->chld) {
    AkHeapNode *toFree;
    AkHeapNode *nextFree;

    toFree = heapNode->chld;

    do {
      nextFree = toFree->next;

      if (toFree->flags & AK_HEAP_NODE_FLAGS_EXT)
        ak_heap_ext_free(heap, toFree);

      /* free attached heap */
      if (toFree->flags & AK_HEAP_NODE_FLAGS_HEAP_CHLD)
        ak_freeh(toFree);
      
      if (toFree->chld) {
        if (heap->trash) {
          AkHeapNode *lastNode;

          lastNode = toFree->chld;
          while (lastNode->next)
            lastNode = lastNode->next;

          lastNode->next = heap->trash;
        }

        heap->trash = toFree->chld;
      }

      alc->free(toFree);
      toFree = nextFree;

      /* empty trash */
      if (!toFree && heap->trash) {
        toFree = heap->trash;
        heap->trash = NULL;
      }

    } while (toFree);
  }

  if (heapNode->prev) {
    if (ak_heap_chld(heapNode->prev) == heapNode)
      ak_heap_chld_set(heapNode->prev, heapNode->next);
    else
      heapNode->prev->next = heapNode->next;
  }

  /* heap->root == heapNode
     we know that root->prev always is NULL */
  else {
    heap->root = heapNode->next;
  }

  if (heapNode->next)
    heapNode->next->prev = heapNode->prev;

  alc->free(heapNode);
}

AK_EXPORT
void
ak_heap_cleanup(AkHeap * __restrict heap) {
  while (heap->root)
    ak_heap_free(heap, heap->root);
}

AK_EXPORT
void *
ak_heap_getId(AkHeap     * __restrict heap,
              AkHeapNode * __restrict heapNode) {
  AkHeapSrchNode *snode;

  if (!(heapNode->flags & AK_HEAP_NODE_FLAGS_SRCH))
    return NULL;

  snode = (AkHeapSrchNode *)((AkHeapNodeExt *)heapNode->chld)->data;
  return snode->key;

  AK__UNUSED(heap);
}

AK_EXPORT
void
ak_heap_setId(AkHeap     * __restrict heap,
              AkHeapNode * __restrict heapNode,
              void       * __restrict memId) {
  AkHeapSrchNode *snode;

  if (!memId) {
    ak_heap_ext_rm(heap,
                   heapNode,
                   AK_HEAP_NODE_FLAGS_SRCH);
    return;
  }

  snode = ak_heap_ext_add(heap,
                          heapNode,
                          AK_HEAP_NODE_FLAGS_SRCH);

  if (snode->key)
    ak_heap_rb_remove(heap->srchctx, snode);

  heapNode->flags |= AK_HEAP_NODE_FLAGS_RED;

  snode->chld[AK__BST_LEFT]  = heap->srchctx->nullNode;
  snode->chld[AK__BST_RIGHT] = heap->srchctx->nullNode;
  snode->key                 = memId;

  ak_heap_rb_insert(heap->srchctx, snode);
}

AK_EXPORT
AkResult
ak_heap_getNodeById(AkHeap      * __restrict heap,
                    void        * __restrict memId,
                    AkHeapNode ** __restrict dest) {
  AkHeapSrchNode *srchNode;

  srchNode = ak_heap_rb_find(heap->srchctx, memId);
  if (!srchNode || srchNode == heap->srchctx->nullNode) {
    *dest = NULL;
    return AK_EFOUND;
  }

  if ((*dest = AK__HEAPNODE(srchNode)))
    return AK_OK;

  return AK_EFOUND;
}

AK_EXPORT
AkResult
ak_heap_getNodeByURL(AkHeap       * __restrict heap,
                     struct AkURL * __restrict url,
                     AkHeapNode  ** __restrict dest) {
  if (url->doc)
    return ak_heap_getNodeById(heap,
                               (char *)url->url + 1,
                               dest);

  return AK_EFOUND;
}

AK_EXPORT
AkResult
ak_heap_getMemById(AkHeap * __restrict heap,
                   void   * __restrict memId,
                   void  ** __restrict dest) {
  AkHeapSrchNode *srchNode;

  srchNode = ak_heap_rb_find(heap->srchctx, memId);
  if (!srchNode || srchNode == heap->srchctx->nullNode) {
    *dest = NULL;
    return AK_EFOUND;
  }

  if ((*dest = ak__alignas(AK__HEAPNODE(srchNode))))
    return AK_OK;

  return AK_EFOUND;
}

AK_EXPORT
int
ak_heap_refc(AkHeapNode * __restrict heapNode) {
  int *refc;

  refc = ak_heap_ext_get(heapNode, AK_HEAP_NODE_FLAGS_REFC);
  if (!refc)
    return -1;

  return *refc;
}

AK_EXPORT
int
ak_heap_retain(AkHeapNode * __restrict heapNode) {
  int *refc;

  refc = ak_heap_ext_get(heapNode, AK_HEAP_NODE_FLAGS_REFC);
  if (!refc)
    refc = ak_heap_ext_add(ak_heap_getheap(ak__alignas(heapNode)),
                           heapNode,
                           AK_HEAP_NODE_FLAGS_REFC);

  return ++(*refc);
}

AK_EXPORT
void
ak_heap_release(AkHeapNode * __restrict heapNode) {
  int *refc;

  refc = ak_heap_ext_get(heapNode, AK_HEAP_NODE_FLAGS_REFC);
  if (!refc || !(*refc))
    goto fr;

  if (--(*refc) > 0)
    return;

fr:
  ak_free(ak__alignas(heapNode));
}

AK_EXPORT
void
ak_heap_printKeys(AkHeap * __restrict heap) {
  ak_heap_rb_print(heap->srchctx);
}

AK_EXPORT
AkHeap*
ak_attachedHeap(void * __restrict memptr) {
  return rb_find(ak__heap_sub, ak__alignof(memptr));
}

AK_EXPORT
void
ak_setAttachedHeap(void   * __restrict memptr,
                   AkHeap * __restrict heap) {
  RBNode     *found;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(memptr);

  if (!heap) {
    rb_remove(ak__heap_sub, heapNode);
    heapNode->flags &= ~AK_HEAP_NODE_FLAGS_HEAP_CHLD;
    return;
  }

  found = rb_find_node(ak__heap_sub, heapNode);
  if (found) {
    found->val = heap;
    return;
  }

  rb_insert(ak__heap_sub, heapNode, heap);

  heapNode->flags |= AK_HEAP_NODE_FLAGS_HEAP_CHLD;
}

AK_EXPORT
AkHeapAllocator *
ak_mem_allocator() {
  return ak__heap.allocator;
}

AK_EXPORT
void
ak_mem_printKeys() {
  ak_heap_rb_print(ak__heap.srchctx);
}

AK_EXPORT
void *
ak_mem_getId(void * __restrict memptr) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(memptr);
  if (heapNode->heapid == 0)
    heap = &ak__heap;
  else
    heap = ak_heap_lt_find(heapNode->heapid);

  return ak_heap_getId(heap, heapNode);
}

AK_EXPORT
void
ak_mem_setId(void * __restrict memptr,
             void * __restrict memId) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(memptr);
  if (heapNode->heapid == 0)
    heap = &ak__heap;
  else
    heap = ak_heap_lt_find(heapNode->heapid);

  ak_heap_setId(heap,
                heapNode,
                memId);
}

AK_EXPORT
AkResult
ak_mem_getMemById(void  * __restrict ctx,
                  void  * __restrict memId,
                  void ** __restrict dest) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(ctx);
  if (heapNode->heapid == 0)
    heap = &ak__heap;
  else
    heap = ak_heap_lt_find(heapNode->heapid);

  return ak_heap_getMemById(heap,
                            memId,
                            dest);
}

AK_EXPORT
void
ak_mem_setp(void * __restrict memptr,
            void * __restrict newparent) {
  ak_heap_setp(ak__alignof(memptr),
               ak__alignof(newparent));
}

AK_EXPORT
void *
ak_mem_parent(void *mem) {
  AkHeapNode *hnode;
  if (!mem)
    return NULL;

  hnode = ak_heap_parent(ak__alignof(mem));
  if (!hnode)
    return NULL;

  return ak__alignas(hnode);
}

AK_EXPORT
void*
ak_malloc(void * __restrict parent,
          size_t size) {
  return ak_heap_alloc(&ak__heap,
                       parent,
                       size);
}

AK_EXPORT
void*
ak_calloc(void * __restrict parent,
          size_t size) {
  void *memptr;

  memptr = ak_heap_alloc(&ak__heap,
                         parent,
                         size);
  memset(memptr, '\0', size);

  return memptr;
}

AK_EXPORT
void*
ak_realloc(void * __restrict parent,
           void * __restrict memptr,
           size_t            newsize) {
  return ak_heap_realloc(&ak__heap,
                          parent,
                          memptr,
                          newsize);
}

AK_EXPORT
char*
ak_strdup(void       * __restrict parent,
          const char * __restrict str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak_heap_alloc(&ak__heap,
                          parent,
                          memsize + 1);

  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

AK_EXPORT
int
ak_refc(void * __restrict mem) {
  return ak_heap_refc(ak__alignof(mem));
}

AK_EXPORT
int
ak_retain(void * __restrict mem) {
  return ak_heap_retain(ak__alignof(mem));
}

AK_EXPORT
void
ak_release(void * __restrict mem) {
  ak_heap_release(ak__alignof(mem));
}

AK_EXPORT
void
ak_free(void * __restrict memptr) {
  AkHeap     *heap;
  AkHeapNode *heapNode;

  if (!memptr)
    return;

  heap     = &ak__heap;
  heapNode = ak__alignof(memptr);

  if (heapNode->heapid != heap->heapid)
    heap = ak_heap_lt_find(heapNode->heapid);

  if (!heap)
    return;

  /* free heap self instead of single free if this node attached to heap */
  if (heap->data == memptr)
    ak_heap_destroy(heap);
  else
    ak_heap_free(heap, heapNode);
}

AK_EXPORT
AkObject*
ak_objAlloc(AkHeap * __restrict heap,
            void   * __restrict memParent,
            size_t              typeSize,
            AkEnum              typeEnum,
            bool                zeroed) {
  AkObject * obj;

  assert(typeSize > 0 && "invalid parameter value");

  obj = ak_heap_alloc(heap,
                      memParent,
                      sizeof(*obj) + typeSize);

  obj->size  = typeSize;
  obj->type  = typeEnum;
  obj->next  = NULL;
  obj->pData = (void *)((char *)obj + offsetof(AkObject, data));

  if (zeroed)
    memset(obj->pData, '\0', typeSize);

  ak_setypeid(obj, AKT_OBJECT);

  return obj;
}

AK_EXPORT
void*
ak_userData(void * __restrict mem) {
  uintptr_t *r, tmp;
  
  if (!mem)
    return NULL;

  if ((r = (uintptr_t *)ak_heap_ext_get(ak__alignof(mem),
                                        AK_HEAP_NODE_FLAGS_USR))) {
    /* to fix 8-byte alignment issue for uintptr_t */
    memcpy(&tmp, r, sizeof(tmp));
    return (void *)tmp;
  }

  return NULL;
}

AK_EXPORT
void*
ak_heap_setUserData(AkHeap * __restrict heap,
                    void   * __restrict mem,
                    void   * __restrict userData) {
  uintptr_t tmp;
  void     *ext;
  
  if (!mem)
    return NULL;
  
  if (!(ext = ak_heap_ext_add(heap, ak__alignof(mem), AK_HEAP_NODE_FLAGS_USR)))
    return NULL;
  
  tmp = (uintptr_t)userData;
  memcpy(ext, &tmp, sizeof(void *));

  return ext;
}

AK_EXPORT
void*
ak_setUserData(void * __restrict mem, void * __restrict userData) {
  AkHeap *heap;
  
  if (!mem || !(heap = ak_heap_getheap(mem)))
    return NULL;

  return ak_heap_setUserData(heap, mem, userData);
}

AK_EXPORT
AkObject*
ak_objFrom(void * __restrict memptr) {
  AkObject *obj;
  obj = (void *)((char *)memptr - offsetof(AkObject, data));
  assert(obj->pData == memptr && "invalid cas to AkObject");
  return obj;
}

void
ak_mem_init() {
  ak__heap_sub = rb_newtree_ptr();
  ak_heap_init(&ak__heap, NULL, NULL, NULL);
  ak_heap_lt_init(&ak__heap);

  ak_dsSetAllocator(ak__heap.allocator, ak__heap_sub->alc);
}

void
ak_mem_deinit() {
  ak_heap_destroy(&ak__heap);
  ak_heap_lt_cleanup();
  rb_destroy(ak__heap_sub);
}
