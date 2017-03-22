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
ak_meshCopyArraysIfNeeded(AkMesh * __restrict mesh) {
  AkHeap           *heap;
  AkObject         *meshobj;
  AkMeshEditHelper *edith;
  AkInput          *input;

  AkMeshPrimitive  *primi;
  AkSource         *src;
  AkAccessor       *acc;
  AkObject         *data, *newdata;
  AkUIntArray      *ind1, *ind2;
  AkUInt           *ind1_it, *ind2_it;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  primi   = mesh->primitive;

  /* per-primitive inputs */
  while (primi) {
    AkSourceArrayBase  *oldarray, *newarray;
    AkSourceArrayState *arrstate;
    void               *arrayid;

    ind1 = primi->indices;
    ind2 = ak_meshIndicesArrayFor(mesh, primi);

    /* same index array */
    if (!ind1 || ind1 == ind2) {
      primi = primi->next;
      continue;
    }

    ind1_it = ind1->items;
    ind2_it = ind2->items;

    input = primi->input;
    while (input) {
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
        goto cont;

      src = ak_getObjectByUrl(&input->base.source);
      if (!src)
        goto cont;

      acc = src->tcommon;
      if (!acc)
        goto cont;

      data = ak_getObjectByUrl(&acc->source);
      if (!data)
        goto cont;

      arrayid  = ak_getId(data);
      arrstate = rb_find(edith->arrays, arrayid);

      /* copy array to mesh */
      if (arrstate) {
        AkSourceEditHelper *srch;
        AkAccessor  *newacc;
        AkDataParam *dp;
        size_t       icount, i;

        newdata  = arrstate->array;
        oldarray = ak_objGet(data);
        newarray = ak_objGet(newdata);

        srch   = ak_meshSourceEditHelper(mesh, &input->base);
        newacc = srch->source->tcommon;
        assert(newacc && "accessor is needed!");

        newacc->firstBound = arrstate->lastoffset;
        ak_accessor_rebound(heap,
                            newacc,
                            arrstate->lastoffset);

        icount = primi->indices->count / primi->indexStride;
        switch (oldarray->type) {
          case AK_SOURCE_ARRAY_TYPE_FLOAT: {
            AkFloat *olditms, *newitms;
            AkUInt   oldindex, newindex, j;

            newitms = newarray->items;
            olditms = oldarray->items;

            for (i = 0; i < icount; i++) {
              j        = 0;
              dp       = acc->param;
              oldindex = ind1_it[i * primi->indexStride + input->offset];
              newindex = ind2_it[i];

              while (dp) {
                if (!dp->name)
                  continue;

                newitms[newindex * arrstate->stride
                        + arrstate->lastoffset
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
          case AK_SOURCE_ARRAY_TYPE_INT: {
            break;
          }
          case AK_SOURCE_ARRAY_TYPE_STRING: {
            break;
          }
          case AK_SOURCE_ARRAY_TYPE_BOOL: {
            break;
          }
          default: break;
        }

        input->offset = 0;
      }

    cont:
      input = (AkInput *)input->base.next;
    }

    primi = primi->next;
  }

  return AK_OK;
}
