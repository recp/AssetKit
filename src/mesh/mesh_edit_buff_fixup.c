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
  AkDataParam        *dp;
  size_t              icount, i;
  AkUInt              oldindex, newindex, j;

  edith   = mesh->edith;
  primi   = mesh->primitive;

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

    input = primi->input;
    while (input) {
      if (input->semantic == AK_INPUT_SEMANTIC_POSITION
          || !(acc     = input->accessor)
          || !(oldbuff = acc->buffer))
        goto cont;

      buffstate = rb_find(edith->buffers, input);

      /* copy buff to mesh */
      if (buffstate) {
        newbuff = buffstate->buff;

        srch   = ak_meshSourceEditHelper(mesh, input);
        newacc = srch->source;
        assert(newacc && "accessor is needed!");

        icount = primi->indices->count / primi->indexStride;
        switch (acc->componentType) {
          case AKT_FLOAT: {
            float *olditms, *newitms;

            newitms = (void *)((char *)newbuff->data + newacc->byteOffset);
            olditms = (void *)((char *)oldbuff->data + acc->byteOffset);

            for (i = 0; i < icount; i++) {
              j        = 0;
              dp       = acc->param;
              oldindex = ind1_it[i * primi->indexStride + input->offset];
              newindex = ind2_it[i];

              while (dp) {
                if (!dp->name)
                  continue;

                /* TODO: byteOffset */
                newitms[newindex * buffstate->stride + j++]
                = olditms[oldindex * acc->stride + dp->offset];

                dp = dp->next;
              }
            }
            break;
          }
          case AKT_INT: {
            break;
          }
          case AKT_STRING: {
            break;
          }
          case AKT_BOOL: {
            break;
          }
          default: break;
        }

        input->offset = 0;
      }

    cont:
      input = input->next;
    }

    primi = primi->next;
  }

  return AK_OK;
}
