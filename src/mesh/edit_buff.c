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

#include "../common.h"
#include "../accessor.h"
#include "../id.h"

#include <assert.h>

#include "edit_common.h"

extern const char* ak_mesh_edit_assert1;

void
ak_meshFreeRsvBuff(RBTree *tree, RBNode *node) {
  AkSourceBuffState *buffstate;

  if (node == tree->nullNode)
    return;

  buffstate = node->val;
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
    buffstate    = ak_heap_calloc(heap, meshobj, sizeof(*buffstate));
    buff         = ak_heap_calloc(heap, meshobj, sizeof(*buff));
    buff->length = newsize;
    buff->data   = ak_heap_calloc(heap, buff, newsize);

    buffstate->buff   = buff;
    buffstate->count  = count;
    buffstate->stride = stride;

    rb_insert(edith->buffers, buffid, buffstate);
    return buffstate;
  }

  buff = buffstate->buff;
  if (buff->length < newsize) {
    buff->data   = ak_heap_realloc(heap, meshobj, buff->data, newsize);
    buff->length = newsize;
  }

  return buffstate;
}

AK_EXPORT
void
ak_meshReserveBufferForInput(AkMesh   * __restrict mesh,
                             AkInput  * __restrict input,
                             size_t                count) {
  AkHeap             *heap;
  AkObject           *meshobj;
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;
  AkSourceBuffState  *buffstate;
  AkAccessor         *acci, *newacc;
  AkBuffer           *buffi;
  void               *buffid;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  if (!(acci = input->accessor)
      || !acci->buffer)
    return;

  /* generate new accesor for input */
  newacc        = ak_accessor_dup(acci);
  newacc->count = (uint32_t)count;

  buffid    = input;
  buffstate = ak_meshReserveBuffer(mesh,
                                   buffid,
                                   acci->componentBytes,
                                   acci->componentCount,
                                   count);
  buffi = buffstate->buff;

  newacc->byteOffset    = 0;
  srch                  = ak_heap_calloc(heap, meshobj, sizeof(*srch));
  srch->oldsource       = acci;
  srch->source          = newacc;
  newacc->buffer        = buffi;

  ak_heap_setpm(newacc, srch->source);

  ak_map_add(edith->inputBufferMap, srch, input);
}

AK_EXPORT
void
ak_meshReserveBuffers(AkMesh          * __restrict mesh,
                      AkMeshPrimitive * __restrict prim,
                      size_t                       count) {
  AkInput *input;

  input = prim->input;
  while (input) {
    ak_meshReserveBufferForInput(mesh, input, count);
    input = input->next;
  }
}

AK_EXPORT
AkSourceEditHelper*
ak_meshSourceEditHelper(AkMesh  * __restrict mesh,
                        AkInput * __restrict input) {
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  srch = (AkSourceEditHelper *)ak_map_find(edith->inputBufferMap, input);

  /* use old source as new */
  if (!srch) {
    /* TODO: */
  }

  return srch;
}

AK_EXPORT
void
ak_meshMoveBuffers(AkMesh * __restrict mesh) {
  AkHeap             *mapHeap;
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;
  AkMapItem          *mi;
  AkInput            *input;
  AkMeshPrimitive    *prim;

  edith   = mesh->edith;
  mapHeap = edith->inputBufferMap->heap;
  mi      = edith->inputBufferMap->root;

  while (mi) {
    input = ak_heap_getId(mapHeap, ak__alignof(mi));
    srch  = (AkSourceEditHelper *)mi->data;
    prim  = ak_mem_parent(input);

    /* TODO */
    // ak_release(input->accessor);

    ak_release(srch->oldsource);
    ak_retain(srch->source);

    input->accessor = srch->source;

    if (input->semantic == AK_INPUT_POSITION)
      prim->pos = input;

    mi = mi->next;
  }
}
