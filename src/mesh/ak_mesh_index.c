/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_index.h"
#include "../../include/ak-trash.h"
#include "ak_mesh_util.h"

#include <limits.h>
#include <assert.h>

extern const char* ak_mesh_edit_assert1;

_assetkit_hide
AkResult
ak_mesh_fix_pos(AkHeap   *heap,
                AkMesh   *mesh,
                AkSource *oldSrc, /* caller alreay has position source */
                uint32_t  newstride) {
  AkInputBasic       *inputb;
  AkMeshEditHelper   *edith;
  AkSourceFloatArray *oldArr;
  AkSource           *src;
  AkObject           *oldData;
  AkAccessor         *acc, *oldAcc;
  AkDataParam        *dparam;
  AkUIntArray        *dupc;
  AkDuplicator       *duplicator;
  size_t              extc, vc, d, s;
  uint32_t            stride;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  oldAcc  = oldSrc->tcommon;
  oldData = ak_getObjectByUrl(&oldAcc->source);
  if (!oldData)
    return AK_ERR;

  stride     = oldAcc->stride;
  oldArr     = ak_objGet(oldData);
  vc         = oldAcc->count;

  duplicator = ak_meshDuplicatorForIndices(mesh);
  dupc       = duplicator->range->dupc;
  extc       = duplicator->dupCount;

  ak_meshFixIndicesArrays(mesh, duplicator);
  ak_meshReserveArrays(mesh, vc + extc);

  inputb = mesh->vertices->input;
  while (inputb) {
    AkSourceArrayBase  *oldarray, *newarray;
    AkSourceArrayState *arrstate;
    AkObject           *data, *newdata;
    void               *arrayid;

    src = ak_getObjectByUrl(&inputb->source);
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
    if (arrstate) {
      AkSourceEditHelper *srch;
      AkAccessor *newacc;
      size_t      i, j;

      newdata  = arrstate->array;
      oldarray = ak_objGet(data);
      newarray = ak_objGet(newdata);

      srch   = ak_meshSourceEditHelper(mesh, inputb);
      newacc = srch->source->tcommon;
      assert(newacc && "accessor is needed!");

      newacc->firstBound = arrstate->lastoffset;
      ak_accessor_rebound(heap,
                          newacc,
                          arrstate->lastoffset);

      switch (oldarray->type) {
        case AK_SOURCE_ARRAY_TYPE_FLOAT: {
          AkFloat *olditms, *newitms;

          newitms = newarray->items;
          olditms = oldarray->items;

          /* copy single-indexed vert positions */
          for (s = i = 0; i < vc; i++) {
            d = dupc->items[i];

            for (j = 0; j <= d; j++) {
              dparam = oldAcc->param;

              while (dparam) {
                if (!dparam->name)
                  continue;

                newitms[(i + j + s) * newstride + dparam->offset]
                  = olditms[i * stride + dparam->offset];

                dparam = dparam->next;
              }
            }

            s += d;
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
    }

  cont:
    inputb = inputb->next;
  }

  return AK_OK;
}

_assetkit_hide
AkResult
ak_mesh_fix_idx_df(AkHeap *heap, AkMesh *mesh) {
  AkSource        *oldSrc;
  AkAccessor      *oldAcc;
  AkArrayList     *ai;
  uint32_t         stride;
  AkResult         ret;

  oldSrc = ak_mesh_pos_src(mesh);
  if (!oldSrc || !oldSrc->tcommon)
    return AK_ERR;

  oldAcc = oldSrc->tcommon;
  if (!oldSrc)
    return AK_ERR;

  ai     = NULL;
  stride = ak_mesh_arr_stride(mesh, &oldAcc->source);
  ret    = ak_mesh_fix_pos(heap,
                           mesh,
                           oldSrc,
                           stride);

  ak_meshCopyArraysIfNeeded(mesh);

  return AK_OK;
}

_assetkit_hide
AkResult
ak_mesh_fix_indices(AkHeap *heap, AkMesh *mesh) {
  AkResult ret;

  ak_meshBeginEdit(mesh);

  /* currently only default option */
  ret = ak_mesh_fix_idx_df(heap, mesh);

  ak_meshEndEdit(mesh);

  return ret;
}
