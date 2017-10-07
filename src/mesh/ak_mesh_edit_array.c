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
ak_meshFreeRsvArray(RBTree *tree, RBNode *node) {
  AkSourceArrayState *arrstate;

  if (node == tree->nullNode)
    return;

  arrstate = node->val;

  tree->alc->free(arrstate->url);
  ak_free(arrstate);
}

AK_EXPORT
AkSourceArrayState*
ak_meshReserveArray(AkMesh * __restrict mesh,
                    void   * __restrict buffid,
                    size_t              itemSize,
                    uint32_t            stride,
                    size_t              acc_count) {
  AkHeap             *heap;
  AkSourceArrayState *arrstate;
  AkBuffer           *buff;
  AkMeshEditHelper   *edith;
  AkObject           *meshobj;
  size_t              newsize, count;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  count   = acc_count * stride;

  if (!(edith->flags & AK_GEOM_EDIT_FLAG_ARRAYS)
      || !edith->arrays) {
    edith->arrays = rb_newtree_str();
    edith->flags |= AK_GEOM_EDIT_FLAG_ARRAYS;
    ak_dsSetAllocator(heap->allocator, edith->arrays->alc);
  }

  arrstate = rb_find(edith->arrays, buffid);
  newsize  = itemSize * count;

  if (!arrstate) {
    arrstate     = ak_heap_calloc(heap, meshobj, sizeof(*arrstate));
    buff         = ak_heap_calloc(heap, meshobj, sizeof(*buff));
    buff->length = newsize;
    buff->data   = ak_heap_calloc(heap, buff, newsize);

    arrstate->array  = buff;
    arrstate->count  = count;
    arrstate->url    = ak_url_string(heap->allocator, buffid);
    arrstate->stride = stride;

    rb_insert(edith->arrays, buffid, arrstate);
    return arrstate;
  }

  buff = arrstate->array;
  if (buff->length < newsize) {
    buff->data = ak_heap_realloc(heap,
                                 meshobj,
                                 buff->data,
                                 newsize);
    buff->length = newsize;
  }

  return arrstate;
}

AK_EXPORT
void
ak_meshReserveArrayForInput(AkMesh       * __restrict mesh,
                            AkInputBasic * __restrict inputb,
                            uint32_t                  inputOffset,
                            size_t                    count) {
  AkHeap             *heap;
  AkObject           *meshobj;
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;
  AkSourceArrayState *arrstate;
  AkSource           *srci;
  AkAccessor         *acci, *newacc;
  AkBuffer           *buffi;
  void               *buffid;
  size_t              usg;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  srci = ak_getObjectByUrl(&inputb->source);
  if (!srci)
    return;

  acci = srci->tcommon;
  if (!acci)
    return;

  buffi = ak_getObjectByUrl(&acci->source);
  if (!buffi)
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
    buffid = (void *)ak_id_gen(heap,
                               NULL,
                               NULL);
  else
    buffid = ak_getId(buffi);

  /* dont detach array */
  if (acci->offset == 0) {
    arrstate = ak_meshReserveArray(mesh,
                                   buffid,
                                   acci->type->size,
                                   acci->bound,
                                   count);
    buffi = arrstate->array;
  } else {
    AkBuffer *foundbuff;

    /* detach array, because we may need to realloc, realloc-ing continued 
     * arrays is more expensive than individual array. But probably sending 
     * to GPU is faster using continuous arrays. Bu we choice the flexibility 
     * here because of maintenance of array. There maybe an option for this 
     * in the future.
     */
    foundbuff = rb_find(edith->detachedArrays, acci);
    if (!foundbuff) {
      arrstate = ak_meshReserveArray(mesh,
                                     buffid,
                                     acci->type->size,
                                     acci->bound,
                                     count);
      buffi = arrstate->array;
      rb_insert(edith->detachedArrays,
                acci,
                buffi);
    } else {
      buffi    = foundbuff;
      arrstate = rb_find(edith->arrays, buffid);
    }

    ak_accessor_rebound(heap, newacc, 0);

    newacc->firstBound = 0;
    newacc->offset     = 0;
    newacc->stride     = newacc->bound;
  }

  if (ak_refc(srci) > usg || acci->offset != 0)
    ak_heap_setpm(buffid, buffi);

  ak_url_init(newacc,
              arrstate->url,
              &newacc->source);

  srch = ak_heap_calloc(heap, meshobj, sizeof(*srch));
  srch->isnew           = ak_refc(srci) > usg || acci->offset != 0;
  srch->oldsource       = srci;
  srch->source          = ak_heap_calloc(heap, meshobj, sizeof(*srci));
  srch->source->buffer  = buffi;
  srch->source->tcommon = newacc;
  srch->arrayid         = buffid;

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

  ak_map_add(edith->inputArrayMap,
             srch,
             inputb);
}

AK_EXPORT
void
ak_meshReserveArrays(AkMesh * __restrict mesh,
                     size_t              count) {
  AkMeshEditHelper *edith;
  AkMeshPrimitive  *prim;
  AkInputBasic     *inputb;
  AkInput          *input;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  if (!mesh->vertices)
    return;

  inputb = mesh->vertices->input;
  while (inputb) {
    ak_meshReserveArrayForInput(mesh, inputb, 0, count);
    inputb = inputb->next;
  }

  prim = mesh->primitive;
  while (prim) {
    input = prim->input;
    while (input) {
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
        input = (AkInput *)input->base.next;
        continue;
      }

      ak_meshReserveArrayForInput(mesh,
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

  srch = (AkSourceEditHelper *)ak_map_find(edith->inputArrayMap,
                                           input);

  /* use old source as new */
  if (!srch) {
    /* TODO: */
  }

  return srch;
}

AK_EXPORT
void
ak_meshMoveArrays(AkMesh * __restrict mesh) {
  AkHeap             *heap, *mapHeap;
  AkMeshEditHelper   *edith;
  AkObject           *meshobj;
  AkSourceEditHelper *srch;
  AkSourceArrayState *arrstate;
  AkMapItem          *mi;
  AkInputBasic       *inputb;
  void               *foundarray;
  AkResult            ret;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  mapHeap = edith->inputArrayMap->heap;
  mi      = edith->inputArrayMap->root;

  while (mi) {
    inputb = ak_heap_getId(mapHeap, ak__alignof(mi));
    srch   = (AkSourceEditHelper *)mi->data;

    /* move array */
    arrstate = rb_find(edith->arrays, srch->arrayid);
    ret = ak_heap_getMemById(heap,
                             srch->arrayid,
                             &foundarray);
    if (ret == AK_OK) {
      if (foundarray != arrstate->array) {
        ak_moveId(foundarray, arrstate->array);
        ak_release(foundarray);
      }
    } else {
      ak_heap_setId(heap,
                    ak__alignof(arrstate->array),
                    srch->arrayid);
    }

    ak_retain(arrstate->array);

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

    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION)
      mesh->positions = ak_mesh_positions(mesh);
    
    mi = mi->next;
  }
}
