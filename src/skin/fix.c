/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fix.h"

static
AkBoneWeights*
ak_skinFixWeightsForPrimitive(AkSkin          * __restrict skin,
                              AkMeshPrimitive * __restrict prim,
                              uint32_t                     primIdx) {
  AkMeshPrimitive *it;
  uint32_t         idx;

  if (!skin || !skin->weights || skin->nPrims == 0)
    return NULL;

  if (prim && prim->mesh) {
    idx = 0;
    for (it = prim->mesh->primitive; it; it = it->next, idx++) {
      if (it != prim)
        continue;

      if (idx < skin->nPrims && skin->weights[idx])
        return skin->weights[idx];

      break;
    }
  }

  if (primIdx < skin->nPrims)
    return skin->weights[primIdx];

  return NULL;
}

AK_HIDE
void
ak_skinFixWeights(AkMesh * __restrict mesh) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkMeshPrimitive *prim;
  AkSkin          *skin;
  FListItem       *skinItem;
  AkBoneWeights   *wl;
  AkBoneWeight    *w, *iw, *old, *oiw;
  AkDuplicator    *dupl;
  AkIndexArray    *dupc, *dupcsum;
  AkAccessor      *acci;
  size_t          *pOldIndex, *wi;
  size_t           vc, d, s, pno, poo, nwsum, newidx, next, tmp, count;
  size_t           oldVertex;
  uint32_t        *nj, i, j, k, vcount, primIndex;

  if (!(skinItem = mesh->skins))
    return;

  heap      = ak_heap_getheap(mesh->geom);
  doc       = ak_heap_data(heap);
  pOldIndex = NULL;

  /* fix every skin that attached to the mesh */
  do {
    skin      = skinItem->data;
    prim      = mesh->primitive;
    primIndex = 0;

    while (prim) {
      if (!(dupl = rb_find(doc->reserved, prim))
          || dupl->dupCount < 1
          || !dupl->range
          || !prim->pos
          || !(acci = prim->pos->accessor))
        goto nxt_prim;

      wl          = ak_skinFixWeightsForPrimitive(skin, prim, primIndex);
      if (!wl || !wl->counts || !wl->indexes || !wl->weights)
        goto nxt_prim;

      old         = wl->weights;
      pOldIndex   = wl->indexes;
      oldVertex   = wl->nVertex;
      vc          = acci->count;
      dupc        = dupl->range->dupc;
      dupcsum     = dupl->range->dupcsum;
      if (!dupc || !dupcsum)
        goto nxt_prim;

      if ((dupc->count / 3) < vc)
        vc = dupc->count / 3;

      nwsum       = 0;

      wl->nVertex = count = dupl->bufCount + dupl->dupCount;
      nj          = ak_heap_alloc(heap, wl, count * sizeof(uint32_t));
      wi          = ak_heap_alloc(heap, wl, count * sizeof(size_t));

#define AK_SKIN_FIX_DISPATCH(OP)                                            \
      do {                                                                  \
        switch (dupc->componentType) {                                       \
          case AKT_UBYTE:                                                    \
            switch (dupcsum->componentType) {                                \
              case AKT_UBYTE:  OP(uint8_t, uint8_t);   break;               \
              case AKT_USHORT: OP(uint8_t, uint16_t);  break;               \
              case AKT_UINT:   OP(uint8_t, AkUInt);    break;               \
              default: break;                                                \
            }                                                               \
            break;                                                          \
          case AKT_USHORT:                                                   \
            switch (dupcsum->componentType) {                                \
              case AKT_UBYTE:  OP(uint16_t, uint8_t);  break;               \
              case AKT_USHORT: OP(uint16_t, uint16_t); break;               \
              case AKT_UINT:   OP(uint16_t, AkUInt);   break;               \
              default: break;                                                \
            }                                                               \
            break;                                                          \
          case AKT_UINT:                                                     \
            switch (dupcsum->componentType) {                                \
              case AKT_UBYTE:  OP(AkUInt, uint8_t);    break;               \
              case AKT_USHORT: OP(AkUInt, uint16_t);   break;               \
              case AKT_UINT:   OP(AkUInt, AkUInt);     break;               \
              default: break;                                                \
            }                                                               \
            break;                                                          \
          default: break;                                                    \
        }                                                                   \
      } while (0)

#define AK_SKIN_FIX_COUNT_PASS(DUPTYPE, SUMTYPE)                            \
      do {                                                                  \
        const DUPTYPE *dupcItems_;                                           \
        const SUMTYPE *sumItems_;                                            \
                                                                            \
        dupcItems_ = (const DUPTYPE *)(const void *)dupc->items;             \
        sumItems_  = (const SUMTYPE *)(const void *)dupcsum->items;          \
        for (i = 0; i < vc; i++) {                                           \
          if ((poo = dupcItems_[3 * i + 2]) == 0)                            \
            continue;                                                        \
          if (poo > oldVertex)                                               \
            continue;                                                        \
                                                                            \
          pno = dupcItems_[3 * i];                                           \
          d   = dupcItems_[3 * i + 1];                                       \
          if (pno >= dupcsum->count)                                         \
            continue;                                                        \
                                                                            \
          s      = sumItems_[pno];                                           \
          vcount = wl->counts[poo - 1];                                      \
                                                                            \
          for (j = 0; j <= d; j++) {                                         \
            newidx = pno + j + s;                                            \
            if (newidx >= count)                                             \
              continue;                                                      \
            wi[newidx] = vcount;                                             \
            nj[newidx] = vcount;                                             \
            nwsum     += vcount;                                             \
          }                                                                 \
        }                                                                   \
      } while (0)

#define AK_SKIN_FIX_COPY_PASS(DUPTYPE, SUMTYPE)                             \
      do {                                                                  \
        const DUPTYPE *dupcItems_;                                           \
        const SUMTYPE *sumItems_;                                            \
                                                                            \
        dupcItems_ = (const DUPTYPE *)(const void *)dupc->items;             \
        sumItems_  = (const SUMTYPE *)(const void *)dupcsum->items;          \
        for (i = 0; i < vc; i++) {                                           \
          if ((poo = dupcItems_[3 * i + 2]) == 0)                            \
            continue;                                                        \
          if (poo > oldVertex)                                               \
            continue;                                                        \
                                                                            \
          pno = dupcItems_[3 * i];                                           \
          d   = dupcItems_[3 * i + 1];                                       \
          if (pno >= dupcsum->count)                                         \
            continue;                                                        \
                                                                            \
          s      = sumItems_[pno];                                           \
          vcount = wl->counts[poo - 1];                                      \
                                                                            \
          for (j = 0; j <= d; j++) {                                         \
            tmp = pno + j + s;                                               \
            if (tmp >= count)                                                \
              continue;                                                      \
                                                                            \
            newidx = wi[tmp];                                                \
                                                                            \
            for (k = 0; k < vcount; k++) {                                   \
              iw         = &w[newidx + k];                                   \
              oiw        = &old[pOldIndex[poo - 1] + k];                    \
              iw->joint  = oiw->joint;                                      \
              iw->weight = oiw->weight;                                     \
            }                                                               \
                                                                            \
            nwsum += vcount;                                                 \
          }                                                                 \
        }                                                                   \
      } while (0)

      /* copy to new location and duplicate if needed */
      AK_SKIN_FIX_DISPATCH(AK_SKIN_FIX_COUNT_PASS);

      /* prepare weight index */
      for (next = j = 0; j < wl->nVertex; j++) {
        tmp   = wi[j];
        wi[j] = next;
        next  = tmp + next;
      }

      /* now we know the size of arrays: weights, pCount, pIndex */
      w     = ak_heap_alloc(heap, wl, sizeof(*w) * nwsum);
      nwsum = 0;

      AK_SKIN_FIX_DISPATCH(AK_SKIN_FIX_COPY_PASS);

#undef AK_SKIN_FIX_COPY_PASS
#undef AK_SKIN_FIX_COUNT_PASS
#undef AK_SKIN_FIX_DISPATCH

      if (pOldIndex)
        ak_free(pOldIndex);

      if (old)
        ak_free(old);

      if (wl->counts)
        ak_free(wl->counts);

      wl->counts  = nj;
      wl->indexes  = wi;
      wl->weights = w;

    nxt_prim:
      primIndex++;
      prim = prim->next;
    }

    skinItem = skinItem->next;
  } while (skinItem);
}
