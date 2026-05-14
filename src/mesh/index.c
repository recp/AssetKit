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

#include "index.h"
#include "../../include/ak/trash.h"

#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

extern const char* ak_mesh_edit_assert1;

typedef struct AkIndexProfile {
  double   collapse;
  double   duplicator;
  double   fixBuffer;
  double   reserveBuffers;
  double   movePositions;
  uint32_t callCount;
  uint32_t skipCount;
  uint32_t collapseCount;
} AkIndexProfile;

static AkIndexProfile ak_index_prof;
static bool           ak_index_prof_active;

static
double
ak_index_profile_now_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

AK_HIDE
void
ak_index_profile_reset(void) {
  memset(&ak_index_prof, 0, sizeof(ak_index_prof));
  ak_index_prof_active = true;
}

AK_HIDE
void
ak_index_profile_report(void) {
  fprintf(stderr,
          "[AssetKit index total] calls=%u skip=%u collapse=%u "
          "collapse_time=%.3fms duplicator=%.3fms fixbuf=%.3fms "
          "reserve=%.3fms move=%.3fms\n",
          ak_index_prof.callCount,
          ak_index_prof.skipCount,
          ak_index_prof.collapseCount,
          ak_index_prof.collapse,
          ak_index_prof.duplicator,
          ak_index_prof.fixBuffer,
          ak_index_prof.reserveBuffers,
          ak_index_prof.movePositions);
  ak_index_prof_active = false;
}

AK_HIDE
bool
ak_primCollapseIdentityIndices(AkMeshPrimitive *prim) {
  AkUIntArray *indices;
  AkInput     *input;
  AkUInt      *items;
  uint32_t     st, vo;
  size_t       count, i, j, base;
  AkUInt       posidx;

  if (!prim
      || prim->indexStride <= 1
      || !(indices = prim->indices)
      || !(items = indices->items)
      || !prim->pos)
    return false;

  st = prim->indexStride;
  vo = prim->pos->offset;
  if (st == 0 || vo >= st || indices->count == 0 || indices->count % st != 0)
    return false;

  count = indices->count / st;
  for (i = 0; i < count; i++) {
    base   = i * st;
    posidx = items[base + vo];
    for (j = 0; j < st; j++) {
      if (items[base + j] != posidx)
        return false;
    }
  }

  for (i = 0; i < count; i++)
    items[i] = items[i * st + vo];

  indices->count    = count;
  prim->indexStride = 1;
  prim->pos->offset = 0;

  for (input = prim->input; input; input = input->next)
    input->offset = 0;

  return true;
}

AK_HIDE
AkResult
ak_movePositions(AkMesh          *mesh,
                 AkMeshPrimitive *prim,
                 AkDuplicator    *duplicator) {
  AkMeshEditHelper   *edith;
  AkSourceEditHelper *srch;
  AkSourceBuffState  *buffstate;
  AkAccessor         *acc, *newacc;
  AkUIntArray        *dupc, *dupcsum;
  AkBuffer           *oldbuff, *newbuff;
  char               *olditms, *newitms;
  size_t              vc, d, s, pno, poo, byteStride;
  uint32_t            i, j;

  if (!prim->pos
      || !(edith     = mesh->edith)
      || !(acc       = prim->pos->accessor)
      || !(oldbuff   = acc->buffer)
      || !(buffstate = rb_find(edith->buffers, prim->pos)))
    return AK_ERR;

  newbuff = buffstate->buff;
  srch    = ak_meshSourceEditHelper(mesh, prim->pos);
  newacc  = srch->source;

  if (!newacc)
    return AK_ERR;

  dupc       = duplicator->range->dupc;
  dupcsum    = duplicator->range->dupcsum;
  vc         = dupc->count;
  newitms    = newbuff->data;
  olditms    = oldbuff->data;
  byteStride = acc->byteStride;

  /* copy vert positions to new location */
  for (i = 0; i < vc; i++) {
    if ((poo = dupc->items[3 * i + 2]) == 0)
      continue;

    pno = dupc->items[3 * i];
    d   = dupc->items[3 * i + 1];
    s   = dupcsum->items[pno];

    for (j = 0; j <= d; j++) {
      memcpy(newitms + byteStride * (pno + j + s),
             olditms + byteStride * (poo - 1),
             byteStride);
    }
  }

  return AK_OK;
}

AK_HIDE
AkResult
ak_primFixIndicesRetainDuplicator(AkMesh          *mesh,
                                  AkMeshPrimitive *prim,
                                  bool             retainDuplicator) {
  AkDuplicator *dupl;
  double        t;

  if (ak_index_prof_active)
    ak_index_prof.callCount++;

  if (prim->indexStride == 1 || !prim->indices) {
    if (ak_index_prof_active)
      ak_index_prof.skipCount++;
    return AK_OK;
  }

  t = ak_index_prof_active ? ak_index_profile_now_ms() : 0.0;
  if (ak_primCollapseIdentityIndices(prim)) {
    if (ak_index_prof_active) {
      ak_index_prof.collapse += ak_index_profile_now_ms() - t;
      ak_index_prof.collapseCount++;
    }
    return AK_OK;
  }
  if (ak_index_prof_active)
    ak_index_prof.collapse += ak_index_profile_now_ms() - t;

  t = ak_index_prof_active ? ak_index_profile_now_ms() : 0.0;
  if (!(dupl = ak_meshDuplicatorForIndicesRetained(mesh,
                                                   prim,
                                                   retainDuplicator)))
    return AK_ERR;
  if (ak_index_prof_active)
    ak_index_prof.duplicator += ak_index_profile_now_ms() - t;

  t = ak_index_prof_active ? ak_index_profile_now_ms() : 0.0;
  ak_meshFixIndexBuffer(mesh, prim, dupl);
  if (ak_index_prof_active)
    ak_index_prof.fixBuffer += ak_index_profile_now_ms() - t;

  t = ak_index_prof_active ? ak_index_profile_now_ms() : 0.0;
  ak_meshReserveBuffers(mesh, prim, dupl->dupCount + dupl->bufCount);
  if (ak_index_prof_active)
    ak_index_prof.reserveBuffers += ak_index_profile_now_ms() - t;

  t = ak_index_prof_active ? ak_index_profile_now_ms() : 0.0;
  ak_movePositions(mesh, prim, dupl);
  if (ak_index_prof_active)
    ak_index_prof.movePositions += ak_index_profile_now_ms() - t;

  if (!retainDuplicator)
    ak_free(dupl);

  return AK_OK;
}

AK_HIDE
AkResult
ak_primFixIndices(AkMesh          *mesh,
                  AkMeshPrimitive *prim) {
  return ak_primFixIndicesRetainDuplicator(mesh, prim, true);
}

AK_HIDE
AkResult
ak_meshFixIndicesDefaultRetainDuplicators(AkMesh *mesh,
                                          bool    retainDuplicators) {
  AkMeshPrimitive *prim;

  prim = mesh->primitive;
  while (prim) {
    ak_primFixIndicesRetainDuplicator(mesh, prim, retainDuplicators);
    prim = prim->next;
  }

  return AK_OK;
}

AK_HIDE
AkResult
ak_meshFixIndicesDefault(AkMesh *mesh) {
  return ak_meshFixIndicesDefaultRetainDuplicators(mesh, true);
}

AK_HIDE
AkResult
ak_meshFixIndices(AkMesh *mesh) {
  AkResult ret;

  ak_meshBeginEdit(mesh);

  /* currently only default option */
  ret = ak_meshFixIndicesDefault(mesh);

  ak_meshEndEdit(mesh);

  return ret;
}
