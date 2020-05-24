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

#include <assert.h>

extern const char* ak_mesh_edit_assert1;

AK_EXPORT
AkResult
ak_meshFillBuffers(AkMesh * __restrict mesh) {
  AkMeshEditHelper   *edith;
  AkInput            *input;
  AkMeshPrimitive    *primi;
  AkAccessor         *acc, *newacc;
  AkUIntArray        *ind1, *ind2;
  AkUInt             *ind1_it, *ind2_it;
  AkBuffer           *oldbuff, *newbuff;
  AkSourceBuffState  *buffstate;
  AkSourceEditHelper *srch;
  char               *olditms, *newitms;
  size_t              icount, i;
  AkUInt              oldidx, newidx;

  edith = mesh->edith;
  primi = mesh->primitive;
 
  /* per-primitive inputs */
  while (primi) {
    ind1 = primi->indices;
    ind2 = ak_meshIndicesArrayFor(mesh, primi, false);

    /* same index buff */
    if (!ind1 || ind1 == ind2) {
      primi = primi->next;
      continue;
    }

    ind1_it = ind1->items;
    ind2_it = ind2->items;
    input   = primi->input;

    while (input) {
      if (input->semantic == AK_INPUT_SEMANTIC_POSITION
          || !(acc     = input->accessor)
          || !(oldbuff = acc->buffer))
        goto cont;

      /* copy buff to mesh */
      if ((buffstate = rb_find(edith->buffers, input))) {
        srch    = ak_meshSourceEditHelper(mesh, input);
        newbuff = buffstate->buff;
        newacc  = srch->source;

        assert(newacc && "accessor is needed!");

        icount  = primi->indices->count / primi->indexStride;
        newitms = (char *)newbuff->data + newacc->byteOffset;
        olditms = (char *)oldbuff->data + acc->byteOffset;
        
        for (i = 0; i < icount; i++) {
          oldidx = ind1_it[i * primi->indexStride + input->offset];
          newidx = ind2_it[i];
          
          memcpy(newitms + newacc->byteStride * newidx,
                 olditms + acc->byteStride    * oldidx,
                 acc->byteStride);
        }

        /* to prevent duplication operation for next time */
        input->offset = 0;
      }

    cont:
      input = input->next;
    }

    primi = primi->next;
  }

  return AK_OK;
}
