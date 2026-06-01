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
  AkBufferEditState *buffstate;

  if (node == tree->nullNode)
    return;

  buffstate = node->val;
  ak_free(buffstate);
}

AK_EXPORT
AkBufferEditState*
ak_meshReserveBuffer(AkMesh * __restrict mesh,
                     void   * __restrict buffid,
                     size_t              itemSize,
                     uint32_t            stride,
                     size_t              acc_count) {
  AkHeap            *heap;
  AkBufferEditState *buffstate;
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
    edith->buffers = rb_newtree_ptr();
    edith->flags |= AK_GEOM_EDIT_FLAG_ARRAYS;
    ak_dsSetAllocator(heap->allocator, edith->buffers->alc);
    edith->buffers->onFreeNode = ak_meshFreeRsvBuff;
  }

  buffstate = rb_find(edith->buffers, buffid);
  newsize   = itemSize * count;

  if (!buffstate) {
    buffstate    = ak_heap_alloc(heap, meshobj, sizeof(*buffstate));
    buff         = ak_heap_alloc(heap, meshobj, sizeof(*buff));
    buff->length = newsize;
    buff->data   = ak_heap_alloc(heap, buff, newsize);
    buff->name   = NULL;

    buffstate->duplicator  = NULL;
    buffstate->buff        = buff;
    buffstate->url         = NULL;
    buffstate->count       = count;
    buffstate->stride      = stride;
    buffstate->next        = NULL;
    buffstate->oldAccessor = NULL;
    buffstate->accessor    = NULL;
    buffstate->input       = NULL;

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
  AkBufferEditState  *buffstate;
  AkAccessor         *acci, *newacc;
  AkBuffer           *buffi;
  size_t              newsize, itemCount;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  if (!(acci = input->accessor)
      || !acci->buffer)
    return;

  itemCount = count * acci->componentCount;
  newsize   = itemCount * acci->bytesPerComponent;
  buffstate = input->reserved;

  if (!buffstate) {
    buffstate     = ak_heap_alloc(heap, meshobj, sizeof(*buffstate));
    buffi         = ak_heap_alloc(heap, meshobj, sizeof(*buffi));
    buffi->length = newsize;
    buffi->data   = ak_heap_alloc(heap, buffi, newsize);
    buffi->name   = NULL;

    buffstate->duplicator  = NULL;
    buffstate->buff        = buffi;
    buffstate->url         = NULL;
    buffstate->count       = itemCount;
    buffstate->stride      = acci->componentCount;
    buffstate->next        = edith->bufferList;
    buffstate->oldAccessor = NULL;
    buffstate->accessor    = NULL;
    buffstate->input       = input;
    edith->bufferList      = buffstate;
    input->reserved        = buffstate;
  } else {
    buffi = buffstate->buff;
    if (buffi->length < newsize) {
      buffi->data   = ak_heap_realloc(heap, meshobj, buffi->data, newsize);
      buffi->length = newsize;
    }

    if (buffstate->accessor)
      return;
  }

  /* generate new accesor for input */
  newacc        = ak_heap_alloc(heap, input, sizeof(*newacc));
  memcpy(newacc, acci, sizeof(*newacc));
  ak_setypeid(newacc, AKT_ACCESSOR);
  newacc->count = (uint32_t)count;

  newacc->byteOffset    = 0;
  /* New buffer is tightly-packed for this input alone (one accessor →
     one buffer). ak_accessor_dup memcpy'd the SOURCE byteStride which
     is wrong here when the source was interleaved (stride > fillByteSize):
     ak_meshFillBuffers would then write at `newByteSt * newidx` past the
     buffer end. Reset to fillByteSize so writes match the layout we
     allocated for. */
  newacc->byteStride    = newacc->fillByteSize;
  newacc->buffer        = buffi;
  newacc->byteLength    = newacc->count * newacc->fillByteSize;
  buffstate->oldAccessor = acci;
  buffstate->accessor    = newacc;
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
void
ak_meshMoveBuffers(AkMesh * __restrict mesh) {
  AkMeshEditHelper   *edith;
  AkBufferEditState  *buffstate, *next;
  AkInput            *input;
  AkMeshPrimitive    *prim;

  edith   = mesh->edith;
  buffstate = edith->bufferList;

  while (buffstate) {
    next  = buffstate->next;
    input = buffstate->input;
    if (!input || !buffstate->accessor) {
      if (input)
        input->reserved = NULL;
      ak_free(buffstate);
      buffstate = next;
      continue;
    }

    prim  = ak_mem_parent(input);

    /* TODO */
    // ak_release(input->accessor);

    ak_release(buffstate->oldAccessor);
    ak_retain(buffstate->accessor);

    input->accessor = buffstate->accessor;
    input->reserved = NULL;

    if (input->semantic == AK_INPUT_POSITION)
      prim->pos = input;

    ak_free(buffstate);
    buffstate = next;
  }

  edith->bufferList = NULL;
}
