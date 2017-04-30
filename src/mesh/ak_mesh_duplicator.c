/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "ak_mesh_index.h"
#include "ak_mesh_util.h"
#include "ak_mesh_edit_common.h"
#include <limits.h>

AK_EXPORT
AkDuplicator*
ak_meshDuplicatorForIndices(AkMesh * __restrict mesh) {
  AkHeap             *heap;
  AkObject           *meshobj;
  AkDuplicator       *duplicator;
  AkDuplicatorRange  *dupr;
  AkUIntArray        *dupc;
  AkInputDesc        *idesc, *idesci, *last_idesc;
  AkMeshPrimitive    *prim, *primi;
  AkUInt             *it, *posflgs;
  AkPrimProxy        *fp, *lfp, *fpi;
  AkInputBasic       *inputb;
  AkSource           *posSource;
  AkAccessor         *posAcc;
  AkSourceFloatArray *posArray;
  AkObject           *posData;
  size_t              count, ccount, icount;
  uint32_t            chk, iter, inpc, inpi;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);

  duplicator = ak_heap_calloc(heap,
                              meshobj,
                              sizeof(*duplicator));
  prim      = mesh->primitive;
  primi     = prim;
  lfp       = NULL;
  fp        = NULL;
  posArray  = NULL;
  posSource = NULL;
  posAcc    = NULL;
  posData   = NULL;

  if (!prim)
    return 0;

  inputb = mesh->vertices->input;
  while (inputb) {
    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
      if (!(posSource  = ak_getObjectByUrl(&inputb->source))
          || !(posAcc  = posSource->tcommon)
          || !(posData = ak_getObjectByUrl(&posAcc->source)))
        return NULL;

      posArray = ak_objGet(posData);
      break;
    }
    inputb = inputb->next;
  }

  /* there is no positions */
  if (!posSource || !posAcc || !posData || !posArray)
    return NULL;

  dupc = ak_heap_calloc(heap,
                        NULL,
                        sizeof(AkUIntArray)
                        + sizeof(AkUInt) * posAcc->count);
  dupc->count = posAcc->count;

  count = icount = ccount = chk = inpc = 0;

  idesc      = NULL;
  last_idesc = NULL;

  /* collect some info */
  while (primi) {
    /* find best stride for single alloc */
    if (primi->inputCount > inpc)
      inpc = primi->inputCount;

    primi = primi->next;
  }

  inpc--;
  inpi = 0;

  primi = prim;
  while (primi) {
    AkInput *input;

    fpi = ak_heap_calloc(heap, NULL, sizeof(*fpi));
    fpi->inpi   = ak_heap_calloc(heap, fpi, sizeof(*fpi->inpi) * inpc);
    fpi->orig   = primi;
    fpi->st     = primi->indexStride;
    fpi->ind    = primi->indices;
    fpi->count  = (uint32_t)fpi->ind->count;
    fpi->icount = (uint32_t)fpi->count / fpi->st;
    fpi->flg    = ak_heap_calloc(heap,
                                 fpi,
                                 sizeof(uint8_t) * fpi->icount);
    fpi->input     = primi->input;
    fpi->newind    = ak_meshIndicesArrayFor(mesh, primi);

    fpi->chk_start = 0;
    fpi->chk_end   = fpi->icount;

    memset(fpi->newind->items,
           0,
           fpi->newind->count * sizeof(AkUInt));

    /* set all inputs' index -1 as default */
    memset(fpi->inpi,
           UINT_MAX,
           sizeof(int32_t) * inpc);

    icount += fpi->icount;
    input   = fpi->input;
    while (input) {
      AkInputDesc *found;

      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
        fpi->vo = input->offset;
        input   = (AkInput *)input->base.next;
        continue;
      }

      found = NULL;
      if (idesc) {
        idesci = idesc;
        while (idesci) {
          if (strcasecmp(idesci->semantic,
                         input->base.semanticRaw) == 0
              && idesci->set == input->set) {
            found = idesci;
            break;
          }

          idesci = idesci->next;
        }
      }

      if (!found) {
        found           = ak_heap_calloc(heap, fpi, sizeof(*found));
        found->semantic = input->base.semanticRaw;
        found->set      = input->set;
        found->source   = &input->base.source; /* ??? */
        found->input    = input;
        found->pp       = fpi;
        found->index    = inpi++;

        if (last_idesc)
          last_idesc->next = found;
        else
          idesc = found;
        last_idesc = found;
      }

      fpi->inpi[found->index] = input->offset;

      input = (AkInput *)input->base.next;
    }

    if (lfp)
      lfp->next = fpi;
    else
      fp = fpi;

    lfp = fpi;

    primi = primi->next;
  }

  posflgs = ak_heap_calloc(heap,
                           NULL,
                           sizeof(uint32_t)
                           * (posArray->base.count * (inpc + 1)));

  iter = 1;
  chk  = 1;
  while (ccount < icount) {
    fpi = fp;
    while (fpi) {
      AkUIntArray *ind;
      AkUInt      *it2;
      uint32_t     st, vo, i, j;

      /* nothing to check */
      if (fpi->chk_start >= fpi->chk_end) {
        fpi = fpi->next;
        continue;
      }

      ind = fpi->ind;
      it  = ind->items;
      it2 = fpi->newind->items;
      st  = fpi->st;
      vo  = fpi->vo;

      j  = fpi->chk_start;
      i  = j * st;

      for (; j < fpi->chk_end; i += st, j++) {
        uint32_t *inp, idxp, k;
        bool      is_eq;

        if (fpi->flg[j] == chk)
          continue;

        idxp  = it[i + vo];
        inp   = posflgs + idxp * (inpc + 1);
        is_eq = true;

        if (inp[0] < iter) {
          /* skip first squence */
          if (iter > 1) {
            dupc->items[idxp]++;
            count++;
          }

          inp[0] = iter;
          for (k = 0; k < inpc; k++) {
            inpi = fpi->inpi[k];
            inp[k + 1] = inpi == UINT_MAX ? UINT_MAX : it[inpi];
          }
        } else {
          for (k = 0; k < inpc; k++) {
            inpi = fpi->inpi[k];
            if (inpi == UINT_MAX)
              continue;

            if (it[i+ inpi] != inp[k + 1]) {
              is_eq = false;
              break;
            }
          }
        }

        if (!is_eq) {
          it2[j]++;
          continue;
        }

        ccount++;
        fpi->flg[j] = chk;

        /* shrink the check range for next iter */
        if (j == fpi->chk_start)
          fpi->chk_start++;
        else if (j == fpi->chk_end)
          fpi->chk_end--;
      }

      fpi = fpi->next;
    }

    iter++;
  }

  dupr             = ak_heap_alloc(heap, duplicator, sizeof(*dupr));
  dupr->dupc       = dupc;
  dupr->startIndex = 0;
  dupr->endIndex   = posAcc->count;
  dupr->next       = NULL;

  duplicator->range    = dupr;
  duplicator->dupCount = count;

  /* cleanup */
  fpi = fp;
  while (fpi) {
    AkPrimProxy *tofree;

    tofree = fpi;
    fpi = fpi->next;

    ak_free(tofree);
  }

  return duplicator;
}

AK_EXPORT
void
ak_meshFixIndicesArrays(AkMesh       * __restrict mesh,
                        AkDuplicator * __restrict duplicator) {
  AkHeap            *heap;
  AkObject          *meshobj;
  AkMeshPrimitive   *prim;
  AkDuplicatorRange *dupr;
  AkUIntArray       *dupc;
  AkObject          *posobj;
  AkSourceArrayBase *posarr;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  dupr    = duplicator->range;
  dupc    = dupr->dupc;
  posobj  = mesh->positions;
  posarr  = ak_objGet(posobj);

  prim    = mesh->primitive;
  if (duplicator->dupCount > 0) {
    AkUIntArray *dupcsum;
    uint32_t     sum, i, j;

    dupcsum = ak_heap_calloc(heap,
                             dupc,
                             sizeof(AkUIntArray)
                               + sizeof(AkUInt) * posarr->count);
    /* shift indices */
    for (sum = i = 0; i < dupc->count; i++) {
      dupcsum->items[i] = sum;
      sum += dupc->items[i];
    }

    while (prim) {
      AkUIntArray *newind;
      AkUInt *it, *it2;
      uint32_t c, st, vo, idxp;

      newind = ak_meshIndicesArrayFor(mesh, prim);
      it     = prim->indices->items;
      it2    = newind->items;
      st     = prim->indexStride;
      vo     = ak_mesh_vertex_off(prim);
      c      = (uint32_t)prim->indices->count;

      for (i = j = 0; i < c; i += st, j++) {
        idxp   = it[i + vo];
        it2[j] = it2[j] + idxp + dupcsum->items[idxp];
      }

      prim = prim->next;
    }
  } else {
    while (prim) {
      AkUIntArray *newind;
      AkUInt *it, *it2;
      uint32_t i, j, c, st, vo;

      newind = ak_meshIndicesArrayFor(mesh, prim);
      it     = prim->indices->items;
      it2    = newind->items;
      st     = prim->indexStride;
      vo     = ak_mesh_vertex_off(prim);
      c      = (uint32_t)prim->indices->count;

      for (i = j = 0; i < c; i += st, j++)
        it2[j] = it[i + vo];

      prim = prim->next;
    }
  }
}

AK_EXPORT
void
ak_meshDuplicatorDuplicate(AkMesh            *mesh,
                           AkDuplicator      *duplicator,
                           AkSourceArrayBase *array,
                           AkSourceArrayBase *newarray) {
}
