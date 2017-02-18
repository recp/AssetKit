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
             AkUIntArray *dupc,
             AkPrimProxy **primproxy) {
  AkInputDesc        *idesc, *idesci, *last_idesc;
  AkMeshPrimitive    *prim, *primi;
  AkUInt             *it, *posflgs;
  AkPrimProxy        *fp, *lfp, *fpi;
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
      posData   = ak_getObjectByUrl(&posSource->tcommon->source);
      posArray  = ak_objGet(posData);
      break;
    }
    inputb = inputb->next;
  }

  /* there is no positions */
  if (!posArray) {
    *primproxy = NULL;
    return 0;
  }

  posflgs = ak_heap_calloc(heap,
                           NULL,
                           sizeof(uint32_t) * (posArray->count + 2));

  count = icount = ccount = chk = 0;

  idesc      = NULL;
  last_idesc = NULL;
  while (primi) {
    AkInput *input;

    fpi = ak_heap_calloc(heap, NULL, sizeof(*fpi));

    fpi->orig      = primi;
    fpi->st        = primi->indexStride;
    fpi->ind       = primi->indices;
    fpi->count     = (uint32_t)fpi->ind->count;
    fpi->icount    = (uint32_t)fpi->count / fpi->st;
    fpi->ccount    = 0;
    fpi->flg       = ak_heap_calloc(heap,
                                    fpi,
                                    sizeof(uint8_t) * fpi->icount);
    fpi->chk_start = 0;
    fpi->chk_end   = fpi->count;
    fpi->input     = primi->input;
    fpi->newind    = ak_heap_alloc(heap,
                                   primi,
                                   sizeof(*fpi->newind)
                                   + sizeof(AkUInt) * fpi->icount);
    fpi->newind->count = fpi->icount;

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
        found->input    = input;
        found->pp       = fpi;
        found->tag      = 0;

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
      fpi->chk_end   = fpi->icount;
      fpi = fpi->next;
    }

    while (ccount < icount) {
      fpi = fp;
      while (fpi) {
        AkInput     *input;
        AkUIntArray *ind;
        AkUInt      *it2;
        uint32_t     st, vo, i, j, ii;

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
              it2[j] = dupc->items[idxp];
            } else {
              it2[j] = 0;
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
            it2[j] = dupc->items[idxp];

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
      AkUInt *it2;
      uint32_t c, st, vo, idxp, newpos;

      it  = fpi->ind->items;
      it2 = fpi->newind->items;
      st  = fpi->st;
      vo  = fpi->vo;
      c   = fpi->count;

      if (idesc) {
        for (i = j = 0; i < c; i += st, j++) {
          idxp       = it[i + vo];
          newpos     = it2[j] + idxp + dupcsum->items[idxp];
          it[i + vo] = newpos;
          it2[j]     = newpos;
        }
      } else {
        for (i = j = 0; i < c; i += st, j++) {
          idxp       = it[i + vo];
          newpos     = idxp + dupcsum->items[idxp];
          it[i + vo] = newpos;
          it2[j]     = newpos;
        }
      }

      fpi = fpi->next;
    }
  } else {
    fpi = fp;
    while (fpi) {
      AkUInt *it2;
      uint32_t i, j, c, st, vo;

      it  = fpi->ind->items;
      it2 = fpi->newind->items;
      st  = fpi->st;
      vo  = fpi->vo;
      c   = fpi->count;

      if (idesc) {
        for (i = j = 0; i < c; i += st, j++)
          it2[j] = it[i + vo] = it[i + vo] + it2[j];
      } else {
        for (i = j = 0; i < c; i += st, j++)
          it2[j] = it[i + vo] = it[i + vo];
      }

      fpi = fpi->next;
    }
  }

  if (primproxy)
    *primproxy = fp;

  return count;
}

_assetkit_hide
AkResult
ak_mesh_fix_pos(AkHeap       *heap,
                AkMesh       *mesh,
                AkSource     *oldSrc, /* caller alreay has position source */
                uint32_t      newstride,
                size_t       *newArrayCount,
                AkArrayList **ai,
                AkPrimProxy **primProxy) {
  AkSourceFloatArray *arr,  *oldArr;
  AkSource           *src;
  AkObject           *data, *oldData;
  AkAccessor         *acc,  *oldAcc;
  AkDataParam        *dparam;
  AkUIntArray        *dupc;
  AkHeapAllocator    *alc;
  char               *arrurl, *newarrayid;
  AkArrayList        *aii;
  size_t              extc, newc, vc, i, j, d, s;
  uint32_t            stride;

  oldAcc  = oldSrc->tcommon;
  oldData = ak_getObjectByUrl(&oldAcc->source);
  if (!oldData)
    return AK_ERR;

  src    = ak_mesh_src(heap, mesh, oldSrc, INT_MAX);
  acc    = src->tcommon;
  stride = oldAcc->stride;
  oldArr = ak_objGet(oldData);

  if (stride == 0) /* TODO: free resources */
    return AK_ERR;

  newc = oldAcc->count;
  vc   = oldAcc->count;
  alc  = heap->allocator;
  dupc = ak_heap_calloc(heap,
                        NULL,
                        sizeof(AkUIntArray) + sizeof(AkUInt) * vc);

  dupc->count = vc;
  extc = ak_ill_verts(heap,
                      mesh,
                      dupc,
                      primProxy);

  if (!*primProxy)
    return AK_ERR;

  data = ak_objAlloc(heap,
                     src,
                     sizeof(*arr)
                       + (vc + extc) * newstride * sizeof(AkFloat),
                     AK_SOURCE_ARRAY_TYPE_FLOAT,
                     false);
  arr            = ak_objGet(data);
  arr->count     = (vc + extc) * newstride;
  arr->digits    = oldArr->digits;
  arr->magnitude = oldArr->magnitude;
  arr->name      = NULL;
  arr->newArray  = NULL;

  oldArr->newArray = ak_heap_alloc(heap,
                                   NULL,
                                   sizeof(*oldArr->newArray));

  /* copy single-indexed vert positions */
  for (s = i = 0; i < vc; i++) {
    d = dupc->items[i];

    for (j = 0; j <= d; j++) {
      dparam = oldAcc->param;

      while (dparam) {
        if (!dparam->name)
          continue;

        arr->items[(i + j + s) * newstride + dparam->offset]
           = oldArr->items[i * stride + dparam->offset];

        dparam = dparam->next;
      }
    }

    s += d;
  }

  acc->stride = newstride;
  acc->offset = 0; /* make position offset 0 */
  acc->count  = newc + extc;

  /* fix params */
  ak_accessor_rebound(heap, acc, 0);

  if (src == oldSrc) {
    arrurl = (char *)acc->source.url;

    aii = ak_heap_alloc(heap, NULL, sizeof(*aii));
    aii->next    = *ai;
    aii->data    = oldData;
    aii->newdata = data;
    *ai          = aii;
  } else {
    newarrayid = (char *)ak_id_gen(heap, data, ak_getId(oldData));
    arrurl     = ak_id_urlstring(alc, newarrayid);

    ak_url_init(acc, arrurl, &acc->source);
    ak_url_unref(&acc->source);
  }

  if (oldArr->name)
    arr->name = ak_heap_strdup(heap,
                               arr,
                               oldArr->name);

  oldArr->newArray->url    = arrurl;
  oldArr->newArray->array  = arr;
  oldArr->newArray->offset = acc->bound;
  oldArr->newArray->stride = newstride;
  oldArr->newArray->count  = acc->count;
  *newArrayCount           = arr->count;

  src->data = data;
  ak_free(dupc);

  return AK_OK;
}

_assetkit_hide
AkResult
ak_mesh_copy_copyarray(AkHeap             *heap,
                       AkMesh             *mesh,
                       AkPrimProxy        *pp,
                       AkSource           *oldsrc,
                       AkSource           *src,
                       AkAccessor         *oldAcc,
                       char               *srcurl,
                       AkSourceFloatArray *oldArray,
                       AkSourceFloatArray *newArray,
                       bool                same) {
  AkHeapAllocator *alc;
  AkAccessor      *acc;
  AkSource        *srci;
  AkPrimProxy     *ppi;

  AK__UNUSED(mesh);

  alc = heap->allocator;
  acc = src->tcommon;

  ppi = pp;
  while (ppi) {
    AkInput *input;
    input = ppi->input;
    while (input) {
      AkUInt      *it, *it2;
      AkDataParam *dparam;
      uint32_t     i, j;

      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX
          || input->base.reserved == 1) {
        input = (AkInput *)input->base.next;
        continue;
      }

      srci = ak_getObjectByUrl(&input->base.source);
      if (srci != oldsrc) {
        input = (AkInput *)input->base.next;
        continue;
      }

      /* fix input source url */
      if (!same) {
        if (input->base.source.url)
          ak_free((char *)input->base.source.url);

        ak_url_init(input, srcurl, &input->base.source);

        /* because this source's array maybe used by another source */
        ak_trash_add(oldsrc);
      }

      /* move items to new array */
      it  = ppi->newind->items;
      it2 = ppi->ind->items;
      for (i = 0; i < ppi->icount; i++) {
        AkUInt index, index2;

        index  = it[i];
        index2 = it2[i * ppi->st + input->offset];

        j      = 0;
        dparam = oldAcc->param;

        /* TODO: must unbound params be zero? */
        while (dparam) {
          if (!dparam->name)
            continue;

          newArray->items[index * acc->stride
                          + acc->offset
                          + acc->firstBound
                          + j++]
          = oldArray->items[index2 * oldAcc->stride
                            + oldAcc->offset
                            + dparam->offset];
          
          dparam = dparam->next;
        }
      }

      input->offset        = 0; /* single index */
      input->base.reserved = 1;
      input = (AkInput *)input->base.next;
    }
    ppi = ppi->next;
  }

  if (!same) {
    /* because this source's array maybe used by another source */
    ak_trash_add(oldsrc);

    alc->free(srcurl);
  }

  return AK_OK;
}

_assetkit_hide
AkResult
ak_mesh_fix_idx_df(AkHeap *heap, AkMesh *mesh) {
  AkSource        *oldSrc;
  AkAccessor      *oldAcc;
  AkPrimProxy     *pp, *ppi;
  AkHeapAllocator *alc;
  AkArrayList     *ai;
  AkInput         *input;
  uint32_t         stride;
  size_t           count;
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
                           stride,
                           &count,
                           &ai,
                           &pp);

  if (ret != AK_OK)
    return ret;

  alc = heap->allocator;

  /* fix every individual semantic index */

  ppi = pp;
  while (ppi) {
    input = ppi->input;
    while (input) {
      AkSource   *srci,  *oldSrci;
      AkAccessor *acci,  *oldAcci;
      AkObject   *datai, *oldDatai;

      if (input->base.reserved == 1) {
        input = (AkInput *)input->base.next;
        continue;
      }

      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
        input = (AkInput *)input->base.next;
        continue;
      }

      oldSrci = ak_getObjectByUrl(&input->base.source);
      if (!oldSrci)
        return AK_ERR; /* TODO: make this soft error, e.g. ignore */

      oldAcci = oldSrci->tcommon;
      if (!oldAcci)
        return AK_ERR; /* TODO: make this soft error, e.g. ignore */

      oldDatai = ak_getObjectByUrl(&oldAcci->source);
      if (!oldDatai)
        return AK_ERR; /* TODO: make this soft error, e.g. ignore */

      /* currently only floats */
      if (oldDatai->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
        AkSourceFloatArray *oldArrayi, *newArrayi;
        AkSourceArrayNew   *newArray;
        char               *srcurl;
        bool                same;

        oldArrayi   = ak_objGet(oldDatai);
        newArray    = oldArrayi->newArray;
        srci        = ak_mesh_src(heap, mesh, oldSrci, INT_MAX);
        acci        = srci->tcommon;
        acci->bound = oldAcci->bound;

        if (newArray) {
          newArrayi        = newArray->array;
          acci->stride     = newArray->stride;
          acci->offset     = 0;
          acci->count      = newArray->count;
          acci->firstBound = newArray->offset;
          ak_accessor_rebound(heap, acci, newArray->offset);

          newArray->offset += acci->bound;
        } else {
          AkArrayList *aii;

          acci->stride     = ak_mesh_arr_stride(mesh, &oldAcci->source);
          acci->count      = count / acci->bound;
          acci->firstBound = 0;
          ak_accessor_rebound(heap, acci, 0);

          newArray = ak_heap_alloc(heap,
                                   NULL,
                                   sizeof(*newArray));

          datai = ak_objAlloc(heap,
                              srci,
                              sizeof(*newArrayi)
                              + count * acci->stride * sizeof(float),
                              AK_SOURCE_ARRAY_TYPE_FLOAT,
                              true);
          newArrayi            = ak_objGet(datai);
          newArrayi->count     = count * acci->stride;
          newArrayi->digits    = oldArrayi->digits;
          newArrayi->magnitude = oldArrayi->magnitude;
          newArrayi->name      = NULL;
          newArrayi->newArray  = NULL;

          if (oldArrayi->name) {
            newArrayi->name = oldArrayi->name;
            ak_heap_setpm(heap,
                          (void *)oldArrayi->name,
                          newArrayi);
          }

          newArray->array  = newArrayi;
          newArray->offset = acci->bound;
          newArray->stride = acci->stride;
          newArray->count  = acci->count;
          newArray->url    = ak_heap_strdup(heap,
                                            newArray,
                                            acci->source.url);

          if (srci == oldSrci) {
            newArray->url = (char *)acci->source.url;

            aii = ak_heap_alloc(heap, NULL, sizeof(*aii));
            aii->next    = ai;
            aii->data    = oldDatai;
            aii->newdata = datai;
            ai           = aii;
          } else {
            char *newarrayid;
            newarrayid   = (char *)ak_id_gen(heap,
                                             datai,
                                             ak_getId(oldDatai));
            newArray->url = ak_id_urlstring(alc, newarrayid);
          }

          oldArrayi->newArray = newArray;
          ak_trash_add(newArray);
        }

        /* update accessor source url */
        ak_url_init(acci, newArray->url, &acci->source);
        ak_url_unref(&acci->source);

        /* we need fix all inputs, copy all data from all primitives */

        /* fix input source url */
        same = srci == oldSrci;
        if (!same) {
          char *srcid;
          srcid  = ak_getId(srci);
          srcurl = ak_id_urlstring(alc, srcid);
        } else {
          srcurl = (char *)input->base.source.url;
        }

        /* copy source to new array for all inputs */
        ak_mesh_copy_copyarray(heap,
                               mesh,
                               pp,
                               oldSrci,
                               srci,
                               oldAcci,
                               srcurl,
                               oldArrayi,
                               newArrayi,
                               same);
      }
      
      input = (AkInput *)input->base.next;
    }
    ppi = ppi->next;
  }

  /* move array's ids to new array,
     because at this point we don't need old arrays anymore,
     and we are going to release them
   */
  while (ai) {
    AkArrayList *aii;
    aii = ai;
    ai  = ai->next;

    ak_moveId(aii->data, aii->newdata);
    ak_free(aii->data);
  }

  /* fix indices and free */
  ppi = pp;
  while (ppi) {
    AkPrimProxy     *tofree;
    AkMeshPrimitive *prim;
    tofree = ppi;

    if (ppi) {
      prim = ppi->orig;
      ak_free(prim->indices);
      prim->indices = ppi->newind;

      /* mark primitive as single index */
      prim->indexStride = 1;

      /* make all offsets 0 */
      input = ppi->input;
      while (input) {
        input->offset = 0;
        input = (AkInput *)input->base.next;
      }
    }

    ppi = ppi->next;
    ak_free(tofree);
  }

  ak_trash_empty();
  return AK_OK;
}

_assetkit_hide
AkResult
ak_mesh_fix_indices(AkHeap *heap, AkMesh *mesh) {
  /* currently only default option */
  return ak_mesh_fix_idx_df(heap, mesh);
}
