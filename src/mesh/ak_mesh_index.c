/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_index.h"
#include "../../include/ak-trash.h"
#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../ak_id.h"
#include "ak_mesh_util.h"
#include <limits.h>

size_t
ak_ill_verts(AkHeap      *heap,
             AkMesh      *mesh,
             AkUIntArray *dupc) {
  AkInputDesc        *idesc, *idesci, *last_idesc;
  AkMeshPrimitive    *prim, *primi;
  AkUInt             *it, *posflgs;
  AkFakePrim         *fp, *lfp, *fpi;
  AkInputBasic       *inputb;
  AkSource           *posSource;
  AkSourceFloatArray *posArray;
  AkObject           *posData;
  size_t              count, ccount, icount;
  uint32_t            chk, iter;

  prim     = mesh->primitive;
  primi    = prim;
  lfp      = NULL;
  fp       = NULL;
  posArray = NULL;

  if (!prim)
    return 0;

  inputb = mesh->vertices->input;
  while (inputb) {
    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
      posSource = ak_getObjectByUrl(&inputb->source);
      posData   = ak_getObjectByUrl(&posSource->techniqueCommon->source);
      posArray  = ak_objGet(posData);
      break;
    }
    inputb = inputb->next;
  }

  posflgs = ak_heap_calloc(heap,
                           NULL,
                           sizeof(uint32_t) * (posArray->count + 2));

  chk = count = icount = ccount = 0;

  idesc      = NULL;
  last_idesc = NULL;
  while (primi) {
    AkInput *input;

    fpi = ak_heap_calloc(heap, NULL, sizeof(*fpi));

    fpi->st        = primi->indexStride;
    fpi->ind       = primi->indices;
    fpi->count     = (uint32_t)primi->indices->count;
    fpi->icount    = (uint32_t)fpi->ind->count / fpi->st;
    fpi->ccount    = 0;
    fpi->flg       = ak_heap_calloc(heap, fpi, sizeof(uint8_t) * fpi->icount);
    fpi->chk_start = 0;
    fpi->chk_end   = fpi->count;
    fpi->input     = primi->input;

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
        found = ak_heap_calloc(heap, fpi, sizeof(*found));
        found->semantic = input->base.semanticRaw;
        found->set      = input->set;
        found->source   = &input->base.source; /* ??? */

        if (last_idesc)
          last_idesc->next = found;
        else
          idesc = found;
        last_idesc = found;
      }

      input = (AkInput *)input->base.next;
    }

    if (lfp)
      lfp->next = fpi;
    else
      fp = fpi;

    lfp = fpi;

    primi = primi->next;
  }

  /* find positions which are not share all attribs same */
  idesci = idesc;
  while (idesci) {
    iter = 1;
    chk  = !chk;

    /* reset check range */
    fpi = fp;
    while (fpi) {
      fpi->chk_start = 0;
      fpi->chk_end   = fpi->count / fpi->st;
      fpi = fpi->next;
    }

    while (ccount < icount) {
      fpi = fp;
      while (fpi) {
        AkInput     *input;
        AkUIntArray *ind;
        uint32_t     st, vo, i, j, ii;

        /* nothing to check */
        if (fpi->chk_start >= fpi->chk_end) {
          fpi = fpi->next;
          continue;
        }

        ind  = fpi->ind;
        it   = ind->items;
        st   = fpi->st;
        vo   = fpi->vo;

        input = fpi->input;
        while (input) {
          if (strcasecmp(input->base.semanticRaw,
                         idesci->semantic) == 0
              && input->set == idesci->set
              && input->base.source.doc == idesci->source->doc
              && strcasecmp(input->base.source.url,
                            idesci->source->url) == 0)
            break;

          input = (AkInput *)input->base.next;
        }

        if (!input) {
          fpi = fpi->next;
          continue;
        }

        ii = input->offset;
        j  = fpi->chk_start;
        i  = j * st;

        for (; j < fpi->chk_end; i += st, j++) {
          uint32_t *inp, idx, idxp;

          if (fpi->flg[j] == chk)
            continue;

          idxp = it[i + vo];
          idx  = it[i + ii];
          inp  = posflgs + idxp * 2;

          if (inp[0] < iter) {
            /* skip first squence */
            if (iter > 1) {
              dupc->items[idxp]++;
              count++;
            }

            inp[0] = iter;
            inp[1] = idx;

            ccount++;
            fpi->flg[j] = chk;

            /* shrink the check range for next iter */
            if (j == fpi->chk_start)
              fpi->chk_start++;
            else if (j == fpi->chk_end)
              fpi->chk_end--;
          } else if (inp[1] == idx) {
            ccount++;
            fpi->flg[j] = chk;

            /* shrink the check range for next iter */
            if (j == fpi->chk_start)
              fpi->chk_start++;
            else if (j == fpi->chk_end)
              fpi->chk_end--;
          }
        }

        fpi = fpi->next;
      }

      iter++;
    }

    idesci = idesci->next;
  }

  if (count > 0) {
    AkUIntArray *dupcsum;
    uint32_t     sum, i, j;

    dupcsum = ak_heap_calloc(heap,
                             dupc,
                             sizeof(AkUIntArray)
                               + sizeof(AkUInt) * dupc->count);

    /* shift indices */
    for (sum = i = 0; i < dupc->count; i++) {
      dupcsum->items[i] = sum;
      sum += dupc->items[i];
    }

    fpi = fp;
    while (fpi) {
      uint32_t c, st, vo, idxp;

      it   = fpi->ind->items;
      st   = fpi->st;
      vo   = fpi->vo;
      c    = fpi->count;

      for (i = j = 0; i < c; i += st, j++) {
        idxp = it[i + vo];
        it[i + vo] += dupcsum->items[idxp];
      }
      
      fpi = fpi->next;
    }
  }

  /* free resources */
  fpi = fp;
  while (fpi) {
    AkFakePrim *tofree;
    tofree = fpi;
    fpi    = fpi->next;
    ak_free(tofree);
  }

  return count;
}

