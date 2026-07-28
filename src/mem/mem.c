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
#include "../thread.h"

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

static
void
ak__heap_stats_alloc(AkHeap * __restrict heap,
                     size_t              size);

static
void
ak__heap_stats_free(AkHeap * __restrict heap);

AK_HIDE
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
static AkMutex ak__heap_sub_lock;

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

  ((char *)memptr)[memsize] = '\0';

  return memptr;
}

static
void
ak__heap_stats_alloc(AkHeap * __restrict heap,
                     size_t              size) {
  heap->stats.allocCalls++;
  heap->stats.allocBytes += (uint64_t)size;
  heap->stats.liveNodes++;

  if (heap->stats.liveNodes > heap->stats.peakNodes)
    heap->stats.peakNodes = heap->stats.liveNodes;
}

static
void
ak__heap_stats_free(AkHeap * __restrict heap) {
  heap->stats.freeCalls++;

  if (heap->stats.liveNodes > 0)
    heap->stats.liveNodes--;
}

static
AkHeapNode *
ak__heap_node_alloc(AkHeapAllocator * __restrict allocator,
                    size_t                       size,
                    size_t                       alignment) {
  AkHeapAlignedPrefix *prefix;
  AkHeapNode          *node;
  uintptr_t            payload;
  void                *allocation;
  size_t               total;

  assert(alignment > 0
         && (alignment & (alignment - 1)) == 0
         && "alignment must be a non-zero power of two");

  if (alignment <= 8) {
    assert(size <= SIZE_MAX - ak__heapnd_sz
           && "allocation size overflow");
    node = allocator->malloc(ak__align(ak__heapnd_sz + size));
    assert(node && "malloc failed");
    node->flags = 0;
    return node;
  }

  assert(size <= SIZE_MAX
                   - ak__heapnd_sz
                   - sizeof(*prefix)
                   - (alignment - 1)
         && "aligned allocation size overflow");

  total      = ak__heapnd_sz
             + size
             + sizeof(*prefix)
             + alignment - 1;
  allocation = allocator->malloc(total);
  assert(allocation && "malloc failed");

  payload = (uintptr_t)allocation
          + sizeof(*prefix)
          + ak__heapnd_sz;
  payload = (payload + alignment - 1) & ~(uintptr_t)(alignment - 1);
  node    = (AkHeapNode *)(payload - ak__heapnd_sz);
  prefix  = (AkHeapAlignedPrefix *)((char *)node - sizeof(*prefix));

  prefix->allocation = allocation;
  prefix->size       = size;
  prefix->alignment  = alignment;
  node->flags        = AK__HEAP_NODE_FLAGS_ALIGNED;

  return node;
}

static
void
ak__heap_node_free(AkHeapAllocator * __restrict allocator,
                   AkHeapNode      * __restrict node) {
  if (node->flags & AK__HEAP_NODE_FLAGS_ALIGNED) {
    AkHeapAlignedPrefix *prefix;
    prefix = (AkHeapAlignedPrefix *)((char *)node - sizeof(*prefix));
    allocator->free(prefix->allocation);
    return;
  }

  allocator->free(node);
}

static
void
ak__heap_node_unlink(AkHeap     * __restrict heap,
                     AkHeapNode * __restrict heapNode) {
  if (heapNode->prev)
    heapNode->prev->next = heapNode->next;
  else if (heapNode->parent)
    ak_heap_chld_set(heapNode->parent, heapNode->next);
  else
    heap->root = heapNode->next;

  if (heapNode->next)
    heapNode->next->prev = heapNode->prev;

  heapNode->parent = NULL;
  heapNode->prev   = NULL;
  heapNode->next   = NULL;
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

  ((char *)memptr)[memsize] = '\0';

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

  ((char *)memptr)[size] = '\0';

  return memptr;
}

AK_EXPORT
AkHeapAllocator *
ak_heap_allocator(AkHeap * __restrict heap) {
  return heap->allocator;
}

AK_EXPORT
AkHeap *
ak_heap_default(void) {
  ak__init();
  return &ak__heap;
}

AK_EXPORT
void
ak_heap_getStats(AkHeap      * __restrict heap,
                 AkHeapStats * __restrict stats) {
  if (!stats)
    return;

  if (!heap) {
    memset(stats, 0, sizeof(*stats));
    return;
  }

  *stats = heap->stats;
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

  ak__init();

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
  memset(&heap->stats, 0, sizeof(heap->stats));
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
ak_heap_aligned_alloc(AkHeap * __restrict heap,
                      void   * __restrict parent,
                      size_t              alignment,
                      size_t              size) {
  AkHeapNode *currNode;
  AkHeapNode *parentNode;

  assert((!parent || heap->heapid == ak__alignof(parent)->heapid)
         && "parent and child mem must use same heap");

  currNode = ak__heap_node_alloc(heap->allocator, size, alignment);

  currNode->typeid = 0;
  currNode->chld   = NULL;
  currNode->heapid = heap->heapid;
  currNode->parent = NULL;
  currNode->prev   = NULL;
  ak__heap_stats_alloc(heap, size);

  if (parent) {
    AkHeapNode *chldNode;

    parentNode = ak__alignof(parent);
    currNode->parent = parentNode;
    chldNode   = ak_heap_chld(parentNode);

    ak_heap_chld_set(parentNode, currNode);
    if (chldNode)
      chldNode->prev = currNode;
    currNode->next = chldNode;
  } else {
    if (heap->root)
      heap->root->prev = currNode;
    currNode->next = heap->root;
    heap->root     = currNode;
  }

  return ak__alignas(currNode);
}

AK_EXPORT
void*
ak_heap_alloc(AkHeap * __restrict heap,
              void   * __restrict parent,
              size_t              size) {
  return ak_heap_aligned_alloc(heap, parent, 8, size);
}

AK_EXPORT
void*
ak_heap_aligned_calloc(AkHeap * __restrict heap,
                       void   * __restrict parent,
                       size_t              alignment,
                       size_t              size) {
  void *memptr;

  heap->stats.callocCalls++;
  heap->stats.callocBytes += (uint64_t)size;

  memptr = ak_heap_aligned_alloc(heap, parent, alignment, size);
  memset(memptr, '\0', size);

  return memptr;
}

AK_EXPORT
void*
ak_heap_calloc(AkHeap * __restrict heap,
               void   * __restrict parent,
               size_t              size) {
  void *memptr;

  heap->stats.callocCalls++;
  heap->stats.callocBytes += (uint64_t)size;

  memptr = ak_heap_aligned_alloc(heap, parent, 8, size);
  memset(memptr, '\0', size);

  return memptr;
}

AK_EXPORT
void*
ak_heap_realloc(AkHeap * __restrict heap,
                void   * __restrict parent,
                void   * __restrict memptr,
                size_t              newsize) {
  AkHeapAlignedPrefix *prefix;
  AkHeapNode *oldNode;
  AkHeapNode *newNode;
  AkHeapNode *chld;
  bool        wasTrash;
  size_t      copySize;
  size_t      userNewSize;

  userNewSize = newsize;
  heap->stats.reallocCalls++;
  heap->stats.reallocBytes += (uint64_t)userNewSize;

  if (!memptr)
    return ak_heap_alloc(heap,
                         parent,
                         newsize);

  oldNode = ak__alignof(memptr);
  wasTrash = heap->trash == oldNode;

  if (newsize == 0) {
    ak_heap_free(heap, oldNode);
    return NULL;
  }

  if (oldNode->flags & AK__HEAP_NODE_FLAGS_ALIGNED) {
    prefix  = (AkHeapAlignedPrefix *)((char *)oldNode - sizeof(*prefix));
    newNode = ak__heap_node_alloc(heap->allocator,
                                  userNewSize,
                                  prefix->alignment);
    copySize = prefix->size < userNewSize ? prefix->size : userNewSize;
    memcpy(newNode, oldNode, ak__heapnd_sz + copySize);
    heap->allocator->free(prefix->allocation);
  } else {
    newsize = ak__heapnd_sz + newsize;
    newsize = ak__align(newsize);
    newNode = heap->allocator->realloc(oldNode, newsize);
    assert(newNode && "realloc failed");
  }

  if (heap->root == oldNode)
    heap->root = newNode;

  if (wasTrash)
    heap->trash = newNode;

  if (newNode->chld) {
    if (newNode->flags & AK_HEAP_NODE_FLAGS_EXT) {
      AkHeapNodeExt *exnode;
      exnode       = newNode->chld;
      exnode->node = newNode;
    }

    chld = ak_heap_chld(newNode);
    while (chld) {
      chld->parent = newNode;
      chld = chld->next;
    }
  }

  if (newNode->prev)
    newNode->prev->next = newNode;
  else if (newNode->parent)
    ak_heap_chld_set(newNode->parent, newNode);
  else if (!wasTrash)
    heap->root = newNode;

  if (newNode->next)
    newNode->next->prev = newNode;

  return ak__alignas(newNode);
}

AK_EXPORT
void *
ak_heap_chld(AkHeapNode *heapNode) {
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
    return ((AkHeapNodeExt *)heapNode->chld)->chld;

  return heapNode->chld;
}

AK_EXPORT
void
ak_heap_chld_set(AkHeapNode * __restrict heapNode,
                 AkHeapNode * __restrict chldNode) {
  if (heapNode->flags & AK_HEAP_NODE_FLAGS_EXT)
    ((AkHeapNodeExt *)heapNode->chld)->chld = chldNode;
  else
    heapNode->chld = chldNode;

  if (chldNode) {
    chldNode->parent = heapNode;
    chldNode->prev   = NULL;
  }
}

AK_EXPORT
AkHeapNode *
ak_heap_parent(AkHeapNode *heapNode) {
  return heapNode->parent;
}

AK_EXPORT
void
ak_heap_setp(AkHeapNode * __restrict heapNode,
             AkHeapNode * __restrict newParent) {
  AkHeap *oldheap, *newheap;

  oldheap = ak_heap_lt_find(heapNode->heapid);
  newheap = ak_heap_lt_find(newParent->heapid);

  if (heapNode == oldheap->trash)
    oldheap->trash = heapNode->next;
  else
    ak__heap_node_unlink(oldheap, heapNode);

  /* move all ids to new heap (if it is different) */
  if (newParent->heapid != heapNode->heapid)
    ak_heap_moveh(heapNode, newheap);

  heapNode->next = ak_heap_chld(newParent);
  ak_heap_chld_set(newParent, heapNode);
  if (heapNode->next)
    heapNode->next->prev = heapNode;
}

AK_HIDE
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

AK_HIDE
void
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

      ak__heap_stats_free(heap);
      ak__heap_node_free(heap->allocator, toFree);
      toFree = nextFree;

      /* empty trash */
      if (!toFree && heap->trash) {
        toFree = heap->trash;
        heap->trash = NULL;
      }

    } while (toFree);
  }

  ak__heap_node_unlink(heap, heapNode);

  ak__heap_stats_free(heap);
  ak__heap_node_free(heap->allocator, heapNode);
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
  AkHeapSrchNode *existing;
  AkHeapSrchNode *snode;

  if (!memId) {
    ak_heap_ext_rm(heap,
                   heapNode,
                   AK_HEAP_NODE_FLAGS_SRCH);
    return;
  }

  existing = ak_heap_rb_find(heap->srchctx, memId);
  if (existing
      && existing != heap->srchctx->nullNode
      && AK__HEAPNODE(existing) != heapNode)
    return;

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

  if (!(refc = ak_heap_ext_get(heapNode, AK_HEAP_NODE_FLAGS_REFC)))
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
  AkHeap *heap;

  ak_mutex_lock(&ak__heap_sub_lock);
  heap = rb_find(ak__heap_sub, ak__alignof(memptr));
  ak_mutex_unlock(&ak__heap_sub_lock);

  return heap;
}

AK_EXPORT
void
ak_setAttachedHeap(void   * __restrict memptr,
                   AkHeap * __restrict heap) {
  RBNode     *found;
  AkHeapNode *heapNode;

  heapNode = ak__alignof(memptr);
  ak_mutex_lock(&ak__heap_sub_lock);

  if (!heap) {
    rb_remove(ak__heap_sub, heapNode);
    heapNode->flags &= ~AK_HEAP_NODE_FLAGS_HEAP_CHLD;
    ak_mutex_unlock(&ak__heap_sub_lock);
    return;
  }

  found = rb_find_node(ak__heap_sub, heapNode);
  if (found) {
    found->val = heap;
    ak_mutex_unlock(&ak__heap_sub_lock);
    return;
  }

  rb_insert(ak__heap_sub, heapNode, heap);

  heapNode->flags |= AK_HEAP_NODE_FLAGS_HEAP_CHLD;
  ak_mutex_unlock(&ak__heap_sub_lock);
}

AK_EXPORT
AkHeapAllocator *
ak_mem_allocator(void) {
  return ak__heap.allocator;
}

AK_EXPORT
void
ak_mem_printKeys(void) {
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
  AkHeap *heap;
  void   *memptr;
  size_t  memsize;

  if (parent) { heap = ak_heap_getheap(parent); }
  else        { heap = &ak__heap;               }

  memsize = strlen(str);
  memptr  = ak_heap_alloc(heap, parent, memsize + 1);

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
  if (!mem) return 0;
  return ak_heap_retain(ak__alignof(mem));
}

AK_EXPORT
void
ak_release(void * __restrict mem) {
  if (!mem) return;
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

  obj = ak_heap_aligned_alloc(heap,
                              memParent,
                              AK_ALIGNOF(AkFloat4),
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
  void      *r;
  uintptr_t  tmp;

  if (!mem)
    return NULL;

  /* Keep `r` as void* (not uintptr_t*) — the storage may be misaligned
     for uintptr_t, and UBSan flags the typed pointer even when the only
     read is a memcpy. memcpy on void* is alignment-agnostic. */
  if ((r = ak_heap_ext_get(ak__alignof(mem), AK_HEAP_NODE_FLAGS_USR))) {
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
ak_mem_init(void) {
  ak_mutex_init(&ak__heap_sub_lock);
  ak__heap_sub = rb_newtree_ptr();
  ak_heap_init(&ak__heap, NULL, NULL, NULL);
  ak_heap_lt_init(&ak__heap);

  ak_dsSetAllocator(ak__heap.allocator, ak__heap_sub->alc);
}

void
ak_mem_deinit(void) {
  ak_heap_destroy(&ak__heap);
  ak_heap_lt_cleanup();
  rb_destroy(ak__heap_sub);
  ak_mutex_destroy(&ak__heap_sub_lock);
}
