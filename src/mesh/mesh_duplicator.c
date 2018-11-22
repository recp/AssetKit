/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../memory_common.h"
#include "mesh_index.h"
#include "mesh_util.h"
#include "mesh_edit_common.h"
#include <limits.h>

AK_EXPORT
AkDuplicator*
ak_meshDuplicatorForIndices(AkMesh          * __restrict mesh,
                            AkMeshPrimitive * __restrict prim) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkObject           *meshobj;
  AkDuplicator       *dupl;
  AkDuplicatorRange  *dupr;
  AkUIntArray        *dupc, *ind, *newind, *dupcsum;
  uint32_t           *it, *it2, *posflgs, *inp;
  AkSource           *posSource;
  AkAccessor         *posAcc;
  AkBuffer           *posBuff;
  uint8_t            *flg;
  size_t              count, ccount, icount, chk_start,
                      chk_end, inpsz, vertc, i, j;
  uint32_t            chk, iter, st, vo, posno, idxp;

  if (!prim->pos
      || !(posSource = ak_getObjectByUrl(&prim->pos->source))
      || !(posAcc    = posSource->tcommon)
      || !(posBuff   = ak_getObjectByUrl(&posAcc->source)))
    return NULL;

  vertc   = posAcc->count;
  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  doc     = ak_heap_data(heap);

  if ((dupl = rb_find(doc->reserved, prim))) {
    rb_remove(doc->reserved, prim);
    ak_free(dupl); /* or cache maybe if mesh is not edited ? */
  }

  dupl = ak_heap_calloc(heap, NULL, sizeof(*dupl));

  /* TODO: cache this for multiple primitives */
  dupc = ak_heap_calloc(heap,
                        dupl,
                        sizeof(AkUIntArray) + sizeof(AkUInt) * vertc * 3);
  dupc->count = posAcc->count;

  st      = prim->indexStride;
  vo      = prim->pos->offset;
  ind     = prim->indices;
  icount  = (uint32_t)ind->count / st;
  newind  = ak_meshIndicesArrayFor(mesh, prim, true);
  chk_end = icount;
  it      = ind->items;
  it2     = newind->items;
  inpsz   = sizeof(AkUInt) * st;

  flg     = ak_heap_calloc(heap, dupl, sizeof(uint8_t) * icount);
  posflgs = ak_heap_calloc(heap,
                           dupl,
                           sizeof(AkUInt) * vertc * (st + 1));

  chk_start = ccount = count = chk = posno = 0;
  iter = chk = 1;
  while (ccount < icount) {
    /* nothing to check */
    if (chk_start >= chk_end)
      break;

    j = chk_start;
    i = j * st;

    for (; j < chk_end; i += st, j++) {
      if (flg[j] == chk)
        continue;

      idxp  = it[i + vo];
      inp   = posflgs + idxp * (st + 1);

      if (inp[0] < iter) {
        /* skip first squence */
        if (iter > 1) {
          dupc->items[3 * idxp + 1]++;
          count++;
        } else {
          dupc->items[3 * idxp]     = posno++;
          dupc->items[3 * idxp + 2] = idxp + 1;
        }

        inp[0] = iter;
        memcpy(&inp[1], &it[i], inpsz);
      } else if (memcmp(&it[i], &inp[1], inpsz) != 0) {
        it2[j]++;
        continue;
      }

      ccount++;
      flg[j] = chk;

      /* shrink the check range for next iter */
      if (j == chk_start)
        chk_start++;
      else if (j == chk_end)
        chk_end--;
    }

    iter++;
  }

  dupcsum = ak_heap_calloc(heap,
                           dupc,
                           sizeof(AkUIntArray)
                              + sizeof(AkUInt) * (posno + 1));
  dupcsum->count = posno;

  for (i = 0; i < dupc->count; i++) {
    uint32_t pno, d;

    if (dupc->items[3 * i + 2] == 0)
      continue;

    pno = dupc->items[i * 3];
    d   = dupc->items[i * 3 + 1];

    dupcsum->items[pno + 1] = d;
  }

  for (i = 1; i < dupcsum->count; i++)
    dupcsum->items[i] += dupcsum->items[i - 1];

  dupr             = ak_heap_alloc(heap, dupl, sizeof(*dupr));
  dupr->dupc       = dupc;
  dupr->startIndex = 0;
  dupr->endIndex   = posAcc->count;
  dupr->next       = NULL;
  dupr->dupcsum    = dupcsum;

  dupl->range      = dupr;
  dupl->dupCount   = count;
  dupl->bufCount   = posno;

  ak_free(flg);
  ak_free(posflgs);

  rb_insert(doc->reserved, prim, dupl);

  return dupl;
}

AK_EXPORT
void
ak_meshFixIndexBuffer(AkMesh          * __restrict mesh,
                      AkMeshPrimitive * __restrict prim,
                      AkDuplicator    * __restrict duplicator) {
  AkDuplicatorRange *dupr;
  AkUIntArray       *dupc, *dupcsum, *newind;
  AkUInt            *it, *it2;
  uint32_t           i, j, c, st, vo, idxp, nidxp;

  dupr    = duplicator->range;
  dupc    = dupr->dupc;
  dupcsum = dupr->dupcsum;

  newind = ak_meshIndicesArrayFor(mesh, prim, true);
  it     = prim->indices->items;
  it2    = newind->items;
  st     = prim->indexStride;
  vo     = prim->pos->offset;
  c      = (uint32_t)prim->indices->count;

  if (duplicator->dupCount > 0) {
    for (i = j = 0; i < c; i += st, j++) {
      idxp   = it[i + vo];
      nidxp  = dupc->items[idxp * 3];
      it2[j] = it2[j] + nidxp + dupcsum->items[nidxp];
    }
  } else {
    for (i = j = 0; i < c; i += st, j++) {
      idxp   = it[i + vo];
      nidxp  = dupc->items[idxp * 3];
      it2[j] = nidxp;
    }
  }
}
