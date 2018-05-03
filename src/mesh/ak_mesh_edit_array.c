/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "ak_mesh_util.h"
#include "../ak_accessor.h"
#include "../ak_id.h"

#include <assert.h>

extern const char* ak_mesh_edit_assert1;

void
ak_meshFreeRsvBuff(RBTree *tree, RBNode *node) {
  AkSourceBuffState *buffstate;

  if (node == tree->nullNode)
    return;

  buffstate = node->val;

  tree->alc->free(buffstate->url);
  ak_free(buffstate);
}

AK_EXPORT
AkSourceBuffState*
ak_meshReserveBuffer(AkMesh * __restrict mesh,
                     void   * __restrict buffid,
                     size_t              itemSize,
                     uint32_t            stride,
                     size_t              acc_count) {
  AkHeap            *heap;
  AkSourceBuffState *buffstate;
  AkBuffer          *buff;
  AkMeshEditHelper  *edith;
  AkObject          *meshobj;
  size_t             newsize, count;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  count   = acc_count * stride;

  if (!(edith->flags & AK_GEOM_EDIT_FLAG_ARRAYS)
      || !edith->buffers) {
    edith->buffers = rb_newtree_str();
    edith->flags |= AK_GEOM_EDIT_FLAG_ARRAYS;
    ak_dsSetAllocator(heap->allocator, edith->buffers->alc);
  }

  buffstate = rb_find(edith->buffers, buffid);
  newsize   = itemSize * count;

  if (!buffstate) {
    buffstate     = ak_heap_calloc(heap, meshobj, sizeof(*buffstate));
    buff         = ak_heap_calloc(heap, meshobj, sizeof(*buff));
    buff->length = newsize;
    buff->data   = ak_heap_calloc(heap, buff, newsize);

    buffstate->buff   = buff;
    buffstate->count  = count;
    buffstate->url    = ak_url_string(heap->allocator, buffid);
    buffstate->stride = stride;

    rb_insert(edith->buffers, buffid, buffstate);
    return buffstate;
  }

  buff = buffstate->buff;
  if (buff->length < newsize) {
    buff->data = ak_heap_realloc(heap,
                                 meshobj,
                                 buff->data,
                                 newsize);
    buff->length = newsize;
  }

  return buffstate;
}

AK_EXPORT
void
ak_meshReserveBufferForInput(AkMesh       * __restrict mesh,
                             AkInputBasic * __restrict inputb,
                             uint32_t                  inputOffset,
                             size_t                    count) {
  AkHeap             *heap;
  AkObject           *meshobj;
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;
  AkSourceBuffState  *buffstate;
  AkSource           *srci;
  AkAccessor         *acci, *newacc;
  AkBuffer           *buffi, *foundbuff;
  void               *buffid;
  int                 usg;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  if (!(srci = ak_getObjectByUrl(&inputb->source))
      || !(acci = srci->tcommon)
      || !(buffi = ak_getObjectByUrl(&acci->source)))
    return;

  /* TODO: analyze accesor, continuous data */
  usg = ak_mesh_src_usg(heap, mesh, srci);
  if (acci->count == count
      && inputOffset == 0
      && acci->offset == 0
      && ak_refc(srci) <= usg)
    return;

  /* generate new accesor for input */
  newacc = ak_accessor_dup(acci);
  newacc->count = count;

  if (ak_refc(srci) > usg || acci->offset != 0)
    buffid = (void *)ak_id_gen(heap, NULL, NULL);
  else
    buffid = ak_getId(buffi);

  /* dont detach buffer */
  /*
  if (acci->offset == 0) {
    buffstate = ak_meshReserveBuffer(mesh,
                                     buffid,
                                     acci->type->size,
                                     acci->bound,
                                     count);
    buffi = buffstate->buff;
  } else {
  */

  /* detach buff, because we may need to realloc, realloc-ing continued
   * buffers is more expensive than individual buff. But probably sending
   * to GPU is faster using continuous buffers. Bu we choice the flexibility
   * here because of maintenance of buff. There maybe an option for this
   * in the future.
   */
  foundbuff = rb_find(edith->detachedBuffers, acci);
  if (!foundbuff) {
    buffstate = ak_meshReserveBuffer(mesh,
                                     buffid,
                                     acci->type->size,
                                     acci->bound,
                                     count);
    buffi = buffstate->buff;
    rb_insert(edith->detachedBuffers,
              acci,
              buffi);
  } else {
    buffi     = foundbuff;
    buffstate = rb_find(edith->buffers, buffid);
  }

  ak_accessor_rebound(heap, newacc, 0);

  newacc->firstBound = 0;
  newacc->offset     = 0;
  newacc->stride     = newacc->bound;
  /* } */

  if (ak_refc(srci) > usg || acci->offset != 0)
    ak_heap_setpm(buffid, buffi);

  ak_url_init(newacc, buffstate->url, &newacc->source);

  srch = ak_heap_calloc(heap, meshobj, sizeof(*srch));
  srch->isnew           = ak_refc(srci) > usg || acci->offset != 0;
  srch->oldsource       = srci;
  srch->source          = ak_heap_calloc(heap, meshobj, sizeof(*srci));
  srch->source->buffer  = buffi;
  srch->source->tcommon = newacc;
  srch->buffid         = buffid;

  if (srch->isnew) {
    void *srcid;
    srcid = (void *)ak_id_gen(heap,
                              srch->source,
                              NULL);

    srch->url = ak_url_string(heap->allocator, srcid);
    ak_heap_setId(heap, ak__alignof(srch->source), srcid);
  } else {
    srch->url = (char *)inputb->source.url;
  }

  ak_heap_setpm(newacc, srch->source);

  ak_map_add(edith->inputBufferMap,
             srch,
             inputb);
}

AK_EXPORT
void
ak_meshReserveBuffers(AkMesh * __restrict mesh,
                      size_t              count) {
  AkMeshPrimitive  *prim;
  AkInput          *input;

  prim = mesh->primitive;
  while (prim) {
    input = prim->input;
    while (input) {
      ak_meshReserveBufferForInput(mesh,
                                   &input->base,
                                   input->offset,
                                   count);
      input = (AkInput *)input->base.next;
    }
    prim = prim->next;
  }
}

AK_EXPORT
AkSourceEditHelper*
ak_meshSourceEditHelper(AkMesh       * __restrict mesh,
                        AkInputBasic * __restrict input) {
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  srch = (AkSourceEditHelper *)ak_map_find(edith->inputBufferMap,
                                           input);

  /* use old source as new */
  if (!srch) {
    /* TODO: */
  }

  return srch;
}

AK_EXPORT
void
ak_meshMoveBuffers(AkMesh * __restrict mesh) {
  AkHeap             *heap, *mapHeap;
  AkMeshEditHelper   *edith;
  AkObject           *meshobj;
  AkSourceEditHelper *srch;
  AkSourceBuffState  *buffstate;
  AkMapItem          *mi;
  AkInputBasic       *inputb;
  void               *foundbuff;
  AkResult            ret;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  mapHeap = edith->inputBufferMap->heap;
  mi      = edith->inputBufferMap->root;

  while (mi) {
    inputb = ak_heap_getId(mapHeap, ak__alignof(mi));
    srch   = (AkSourceEditHelper *)mi->data;

    /* move buff */
    buffstate = rb_find(edith->buffers, srch->buffid);
    ret = ak_heap_getMemById(heap,
                             srch->buffid,
                             &foundbuff);
    if (ret == AK_OK) {
      if (foundbuff != buffstate->buff) {
        ak_moveId(foundbuff, buffstate->buff);
        ak_release(foundbuff);
      }
    } else {
      ak_heap_setId(heap,
                    ak__alignof(buffstate->buff),
                    srch->buffid);
    }

    ak_retain(buffstate->buff);

    /* move source */
    if (!srch->isnew) {
      void *foundsource;
      ret = ak_heap_getMemById(heap,
                               (char *)srch->url + 1,
                               &foundsource);
      if (ret == AK_OK)
        ak_moveId(srch->oldsource, srch->source);
    }

    ak_url_unref(&inputb->source);
    if (inputb->source.url != srch->url)
      ak_free((char *)inputb->source.url);

    ak_release(srch->oldsource);
    ak_retain(srch->source);

    ak_url_init(inputb,
                srch->url,
                &inputb->source);

    mi = mi->next;
  }
}
