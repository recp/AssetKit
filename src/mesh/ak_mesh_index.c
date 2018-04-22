/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_index.h"
#include "../../include/ak/trash.h"
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
  AkSource           *src;
  AkAccessor         *acc, *oldAcc;
  AkDataParam        *dparam;
  AkUIntArray        *dupc;
  AkDuplicator       *duplicator;
  size_t              extc, vc, d, s;
  uint32_t            stride;

  edith = mesh->edith;
  assert(edith && ak_mesh_edit_assert1);

  oldAcc = oldSrc->tcommon;
  if (!ak_getObjectByUrl(&oldAcc->source))
    return AK_ERR;

  stride     = oldAcc->stride;
  vc         = oldAcc->count;

  duplicator = ak_meshDuplicatorForIndices(mesh);
  dupc       = duplicator->range->dupc;
  extc       = duplicator->dupCount;

  ak_meshFixIndexBuffer(mesh, duplicator);
  ak_meshReserveBuffers(mesh, vc + extc);

  inputb = mesh->vertices->input;
  while (inputb) {
    AkBuffer          *oldbuff, *newbuff;
    AkSourceBuffState *buffstate;
    void              *buffid;

    src = ak_getObjectByUrl(&inputb->source);
    if (!src)
      goto cont;

    acc = src->tcommon;
    if (!acc)
      goto cont;

    oldbuff = ak_getObjectByUrl(&acc->source);
    if (!oldbuff)
      goto cont;

    buffid   = ak_getId(oldbuff);
    buffstate = rb_find(edith->buffers, buffid);
    if (buffstate) {
      AkSourceEditHelper *srch;
      AkAccessor *newacc;
      size_t      i, j;

      newbuff  = buffstate->buff;
      srch     = ak_meshSourceEditHelper(mesh, inputb);
      newacc   = srch->source->tcommon;
      assert(newacc && "accessor is needed!");

      newacc->firstBound = buffstate->lastoffset;
      ak_accessor_rebound(heap,
                          newacc,
                          buffstate->lastoffset);

      switch (acc->itemTypeId) {
        case AKT_FLOAT: {
          AkFloat *olditms, *newitms;

          newitms = newbuff->data;
          olditms = oldbuff->data;

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
    }

  cont:
    inputb = inputb->next;
  }

  return AK_OK;
}

_assetkit_hide
AkResult
ak_mesh_fix_idx_df(AkHeap *heap, AkMesh *mesh) {
  AkSource   *oldSrc;
  AkAccessor *oldAcc;
  uint32_t    stride;

  oldSrc = ak_mesh_pos_src(mesh);
  if (!oldSrc || !oldSrc->tcommon)
    return AK_ERR;

  oldAcc = oldSrc->tcommon;
  if (!oldAcc)
    return AK_ERR;

  stride = ak_mesh_arr_stride(mesh, &oldAcc->source);
  (void)ak_mesh_fix_pos(heap,
                        mesh,
                        oldSrc,
                        stride);

  ak_meshCopyBuffersIfNeeded(mesh);

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
