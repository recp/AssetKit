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

#include "../common.h"
#include "index.h"
#include "edit_common.h"
#include <limits.h>

static
uint32_t
ak_indexTupleHash(const AkUInt * __restrict tuple,
                  uint32_t                  stride) {
  uint32_t hash, i;

  hash = 2166136261u;

#define AK_INDEX_HASH_STEP(IDX) \
  do {                          \
    hash ^= tuple[IDX];         \
    hash *= 16777619u;          \
  } while (0)

  switch (stride) {
    case 1:
      AK_INDEX_HASH_STEP(0);
      break;
    case 2:
      AK_INDEX_HASH_STEP(0);
      AK_INDEX_HASH_STEP(1);
      break;
    case 3:
      AK_INDEX_HASH_STEP(0);
      AK_INDEX_HASH_STEP(1);
      AK_INDEX_HASH_STEP(2);
      break;
    case 4:
      AK_INDEX_HASH_STEP(0);
      AK_INDEX_HASH_STEP(1);
      AK_INDEX_HASH_STEP(2);
      AK_INDEX_HASH_STEP(3);
      break;
    default:
      for (i = 0; i < stride; i++) {
        AK_INDEX_HASH_STEP(i);
      }
      break;
  }

#undef AK_INDEX_HASH_STEP

  return hash ? hash : 1u;
}

static
bool
ak_indexTupleEq(const AkUInt * __restrict a,
                const AkUInt * __restrict b,
                uint32_t                  stride) {
  switch (stride) {
    case 1:
      return a[0] == b[0];
    case 2:
      return a[0] == b[0] && a[1] == b[1];
    case 3:
      return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
    case 4:
      return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
    default:
      return memcmp(a, b, sizeof(*a) * stride) == 0;
  }
}

static
size_t
ak_indexHashCap(size_t count) {
  size_t cap, need;

  need = count < (SIZE_MAX / 2) ? count * 2 : count;
  cap  = 16;

  while (cap < need && cap < (SIZE_MAX / 2))
    cap <<= 1;

  return cap;
}

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
  uint32_t           *it, *it2, *hashes;
  AkAccessor         *posAcc;
  size_t              count, hcap, hmask, hpos, hslot, icount,
                      vertc, i, j;
  uint32_t            st, vo, posno, idxp, ord;

  if (!prim->pos || !(posAcc = prim->pos->accessor))
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
  it      = ind->items;
  it2     = newind->items;

  hcap   = ak_indexHashCap(icount);
  hmask  = hcap - 1;
  hashes = ak_heap_calloc(heap, dupl, sizeof(uint32_t) * hcap);

  count = posno = 0;
  for (j = i = 0; j < icount; j++, i += st) {
    idxp = it[i + vo];
    if (idxp >= vertc)
      goto fail;

    hpos = ak_indexTupleHash(&it[i], st) & hmask;
    for (;;) {
      hslot = hashes[hpos];
      if (!hslot) {
        if (dupc->items[3 * idxp + 2] == 0) {
          dupc->items[3 * idxp]     = posno++;
          dupc->items[3 * idxp + 2] = idxp + 1;
          ord = 0;
        } else {
          ord = ++dupc->items[3 * idxp + 1];
          count++;
        }

        hashes[hpos] = (uint32_t)j + 1;
        it2[j]       = ord;
        break;
      }

      hslot--;
      if (ak_indexTupleEq(&it[i], &it[hslot * st], st)) {
        it2[j] = it2[hslot];
        break;
      }

      hpos = (hpos + 1) & hmask;
    }
  }

  dupcsum = ak_heap_calloc(heap,
                           dupc,
                           sizeof(AkUIntArray) + sizeof(AkUInt) * (posno + 1));
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

  ak_free(hashes);

  rb_insert(doc->reserved, prim, dupl);

  return dupl;

fail:
  ak_free(dupl);
  return NULL;
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
