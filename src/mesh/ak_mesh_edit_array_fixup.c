/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "ak_mesh_util.h"

#include <assert.h>

extern const char* ak_mesh_edit_assert1;

AK_EXPORT
AkResult
ak_meshFillBuffers(AkMesh * __restrict mesh) {
  AkHeap             *heap;
  AkObject           *meshobj;
  AkMeshEditHelper   *edith;
  AkInput            *input;
  AkMeshPrimitive    *primi;
  AkSource           *src;
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
  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
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
          || !(src     = ak_getObjectByUrl(&input->source))
          || !(acc     = src->tcommon)
          || !(oldbuff = ak_getObjectByUrl(&acc->source)))
        goto cont;

      buffstate = rb_find(edith->buffers, input);

      /* copy buff to mesh */
      if (buffstate) {
        newbuff = buffstate->buff;

        srch   = ak_meshSourceEditHelper(mesh, input);
        newacc = srch->source->tcommon;
        assert(newacc && "accessor is needed!");

        newacc->firstBound = buffstate->lastoffset;
        ak_accessor_rebound(heap,
                            newacc,
                            buffstate->lastoffset);

        icount = primi->indices->count / primi->indexStride;
        switch (acc->itemTypeId) {
          case AKT_FLOAT: {
            AkFloat *olditms, *newitms;

            newitms = newbuff->data;
            olditms = oldbuff->data;

            for (i = 0; i < icount; i++) {
              j        = 0;
              dp       = acc->param;
              oldindex = ind1_it[i * primi->indexStride + input->offset];
              newindex = ind2_it[i];

              while (dp) {
                if (!dp->name)
                  continue;

                newitms[newindex * buffstate->stride
                        + buffstate->lastoffset
                        + newacc->firstBound
                        + newacc->offset
                        + j++]
                = olditms[oldindex * acc->stride
                          + acc->offset
                          + dp->offset];

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
