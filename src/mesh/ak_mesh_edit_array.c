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

AK_EXPORT
AkSourceArrayState*
ak_meshReserveArray(AkMesh * __restrict mesh,
                    void   * __restrict arrayid,
                    AkSourceArrayType   type,
                    uint32_t            stride,
                    size_t              acc_count) {
  AkHeap             *heap;
  AkObject           *data;
  AkSourceArrayState *arrstate;
  AkSourceArrayBase  *array;
  AkMeshEditHelper   *edith;
  AkObject           *meshobj;
  size_t             newsize, arraysize, isize, count;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  count = acc_count * stride;
  if (!(edith->flags & AK_GEOM_EDIT_FLAG_ARRAYS)
      || !edith->arrays) {
    edith->arrays = rb_newtree(ak_cmp_str, NULL);
    edith->flags |= AK_GEOM_EDIT_FLAG_ARRAYS;
  }

  meshobj  = ak_objFrom(mesh);
  heap     = ak_heap_getheap(meshobj);
  arrstate = rb_find(edith->arrays, arrayid);

  arraysize = ak_sourceArraySize(type);
  isize     = ak_sourceArrayItemSize(type);
  newsize   = sizeof(*data)
                + arraysize
                + isize * count;

  if (!arrstate) {
    arrstate = ak_heap_calloc(heap, meshobj, sizeof(*arrstate));
    data = ak_objAlloc(heap,
                       meshobj,
                       newsize,
                       type,
                       false);
    memset(data->data, '\0', arraysize);

    arrstate->array  = data;
    arrstate->count  = count;
    arrstate->url    = ak_id_urlstring(heap->allocator, arrayid);
    arrstate->stride = stride;

    array = ak_objGet(data);
    array->count = count;
    array->type  = type;
    array->items = ak_sourceArrayItems(array);

    rb_insert(edith->arrays, arrayid, arrstate);
    return arrstate;
  }

  data  = arrstate->array;
  array = ak_objGet(data);
  if (array->count < count) {
    data = ak_heap_realloc(heap,
                           meshobj,
                           data,
                           newsize);

    array = ak_objGet(data);
    array->count = count;
    array->type  = type;

    arrstate->array = data;
  }

  return arrstate;
}

AK_EXPORT
AkSourceArrayState*
ak_meshReserveArrayFor(AkMesh   * __restrict mesh,
                       AkObject * __restrict olddata) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkObject          *meshobj;
  void              *arrayid;
  AkURL              arrayURL;
  size_t             count;
  uint32_t           stride;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  doc     = ak_heap_data(heap);
  arrayid = ak_getId(olddata);

  arrayURL.doc = doc;
  arrayURL.url = ak_id_urlstring(heap->allocator, arrayid);

  ak_meshInspectArray(mesh,
                      &arrayURL,
                      &stride,
                      &count);

  return ak_meshReserveArray(mesh,
                             arrayid,
                             olddata->type,
                             stride,
                             count);
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
  AkObject           *datai;
  void               *arrayid;
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

  datai = ak_getObjectByUrl(&acci->source);
  if (!datai)
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
    arrayid = (void *)ak_id_gen(heap,
                                NULL,
                                NULL);
  else
    arrayid = ak_getId(datai);

  /* dont detach array */
  if (acci->offset == 0) {
    arrstate = ak_meshReserveArray(mesh,
                                   arrayid,
                                   datai->type,
                                   acci->bound,
                                   count);
    datai = arrstate->array;
  } else {
    AkObject *founddata;

    /* detach array, because we may need to realloc, realloc-ing continued 
     * arrays is more expensive than individual array. But probably sending 
     * to GPU is faster using continuous arrays. Bu we choice the flexibility 
     * here because of maintenance of array. There maybe an option for this 
     * in the future.
     */
    founddata = rb_find(edith->detachedArrays, acci);
    if (!founddata) {
      arrstate = ak_meshReserveArray(mesh,
                                     arrayid,
                                     datai->type,
                                     acci->bound,
                                     count);
      datai = arrstate->array;
      rb_insert(edith->detachedArrays,
                acci,
                datai);
    } else {
      datai    = founddata;
      arrstate = rb_find(edith->arrays, arrayid);
    }

    ak_accessor_rebound(heap, newacc, 0);

    newacc->firstBound = 0;
    newacc->offset     = 0;
    newacc->stride     = newacc->bound;
  }

  if (ak_refc(srci) > usg || acci->offset != 0)
    ak_heap_setpm(heap, arrayid, datai);

  ak_url_init(newacc,
              arrstate->url,
              &newacc->source);

  srch = ak_heap_calloc(heap, meshobj, sizeof(*srch));
  srch->isnew           = ak_refc(srci) > usg || acci->offset != 0;
  srch->url             = (char *)inputb->source.url;
  srch->oldsource       = srci;
  srch->source          = ak_heap_calloc(heap, meshobj, sizeof(*srci));
  srch->source->data    = datai;
  srch->source->tcommon = newacc;
  srch->arrayid         = arrayid;

  ak_heap_setpm(heap, newacc, srch->source);

  ak_map_add(edith->inputArrayMap,
             &srch,
             sizeof(srch),
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

  srch = *(AkSourceEditHelper **)ak_map_find(edith->inputArrayMap,
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
  void               *founditem;
  AkResult            ret;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  mapHeap = edith->inputArrayMap->heap;
  mi      = edith->inputArrayMap->root;

  while (mi) {
    inputb = ak_heap_getId(mapHeap, ak__alignof(mi));
    srch   = *(AkSourceEditHelper **)mi->data;

    /* move array */
    arrstate = rb_find(edith->arrays, srch->arrayid);
    ret = ak_heap_getMemById(heap,
                             srch->arrayid,
                             &founditem);
    if (ret == AK_OK) {
      if (founditem != arrstate->array) {
        ak_moveId(founditem, arrstate->array);
        ak_free(founditem);
      }
    } else {
      ak_heap_setId(heap,
                    ak__alignof(arrstate->array),
                    srch->arrayid);
    }

    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION)
      mesh->positions = arrstate->array;

    /* move source */
    if (inputb->source.url != srch->url)
      ak_free((char *)inputb->source.url);

    ak_url_init(inputb,
                srch->url,
                &inputb->source);

    mi = mi->next;
  }
}
