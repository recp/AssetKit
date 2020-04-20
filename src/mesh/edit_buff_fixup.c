/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
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
