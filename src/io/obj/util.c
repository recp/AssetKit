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

#include "util.h"

#define wobj_real_index(count, val) val > 0 ? val - 1 : count - val

AK_HIDE
AkAccessor*
wobj_acc(WOState         * __restrict wst,
         AkDataContext   * __restrict dctx,
         AkComponentSize              compSize,
         AkTypeId                     type) {
  AkHeap     *heap;
  AkBuffer   *buff;
  AkAccessor *acc;
  AkTypeDesc *typeDesc;
  int         nComponents;

  heap        = wst->heap;
  typeDesc    = ak_typeDesc(type);
  nComponents = (int)compSize;

  buff         = ak_heap_calloc(heap, wst->doc, sizeof(*buff));
  buff->data   = ak_heap_alloc(heap, buff, dctx->usedsize);
  buff->length = dctx->usedsize;
  ak_data_join(dctx, buff->data, 0, 0);
  
  flist_sp_insert(&wst->doc->lib.buffers, buff);
  
  acc                    = ak_heap_calloc(heap, wst->doc, sizeof(*acc));
  acc->buffer            = buff;
  acc->byteLength        = buff->length;
  acc->byteStride        = typeDesc->size * nComponents;
  acc->componentSize     = compSize;
  acc->componentType     = type;
  acc->bytesPerComponent = typeDesc->size;
  acc->componentCount    = nComponents;
  acc->fillByteSize      = typeDesc->size * nComponents;
  acc->count             = (uint32_t)dctx->itemcount;

  return acc;
}

AK_HIDE
AkInput*
wobj_input(WOState         * __restrict wst,
           AkMeshPrimitive * __restrict prim,
           AkAccessor      * __restrict acc,
           AkInputSemantic              sem,
           const char      * __restrict semRaw,
           uint32_t                     offset) {
  AkInput *inp;

  inp                 = ak_heap_calloc(wst->heap, prim, sizeof(*inp));
  inp->accessor       = acc;
  inp->semantic       = sem;
  inp->semanticRaw    = ak_heap_strdup(wst->heap, inp, semRaw);
  inp->offset         = offset;

  inp->next   = prim->input;
  prim->input = inp;
  prim->inputCount++;
  
  ak_retain(acc);

  return inp;
}

AK_HIDE
void
wobj_joinIndices(WOState         * __restrict wst,
                 WOPrim          * __restrict wp,
                 AkMeshPrimitive * __restrict prim) {
  AkDataChunk *chunk;
  AkUInt      *it, *it2, val;
  size_t       count;
  size_t       isz, csz, i;
  uint32_t     istride, count_pos, count_tex, count_nor;

  if (!wp->dc_face->data)
    return;
  
  count   = wp->dc_face->itemcount;
  istride = 1;

  count_pos = wst->ac_pos->count;
  count_tex = wst->ac_tex->count;
  count_nor = wst->ac_nor->count;

  if (wp->hasTexture || wp->hasNormal) {
    istride += (int)wp->hasNormal + (int)wp->hasTexture;
    count   *= istride;
  }

  prim->indices = ak_heap_calloc(wst->heap,
                                prim,
                                sizeof(*prim->indices) + count * sizeof(AkUInt));
  prim->indices->count = count;
  prim->indexStride    = istride;

  it = prim->indices->items;

  /* join index buffer chunks */
  isz   = wp->dc_face->itemsize;
  chunk = wp->dc_face->data;

  /* to make it faster split cases */
  if (wp->hasNormal && wp->hasTexture) {
    while (chunk) {
      csz = chunk->usedsize;
      it2 = (void *)chunk->data;

      for (i = 0; i < csz; i += isz) {
        /* position */
        val = *it2;
        *it = wobj_real_index(count_pos, val);

        /* texture */
        val = *(it2 + 1);
        *(it + 1) = wobj_real_index(count_tex, val);

        /* normal */
        val = *(it2 + 2);
        *(it + 2) = wobj_real_index(count_nor, val);

        it  += 3;
        it2 += 3;
      }
      chunk = chunk->next;
    }
  } else if (wp->hasNormal) {
    while (chunk) {
      csz = chunk->usedsize;
      it2 = (void *)chunk->data;

      for (i = 0; i < csz; i += isz) {
        /* position */
        val = *it2;
        *it = wobj_real_index(count_pos, val);
        
        /* normal */
        val = *(it2 + 2);
        *(it + 1) = wobj_real_index(count_nor, val);

        it  += 2;
        it2 += 3;
      }
      chunk = chunk->next;
    }
  } else if (wp->hasTexture) {
    while (chunk) {
      csz = chunk->usedsize;
      it2 = (void *)chunk->data;

      for (i = 0; i < csz; i += isz) {
        /* position */
        val = *it2;
        *it = wobj_real_index(count_pos, val);
        
        /* texture */
        val = *(it2 + 1);
        *(it + 1) = wobj_real_index(count_tex, val);

        it  += 2;
        it2 += 3;
      }
      chunk = chunk->next;
    }
  } else {
    while (chunk) {
      csz = chunk->usedsize;
      it2 = (void *)chunk->data;

      for (i = 0; i < csz; i += isz) {
        /* position */
        val = *it2;
        *it = wobj_real_index(count_pos, val);

        it  += 1;
        it2 += 3;
      }
      chunk = chunk->next;
    }
  }
}
