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

#include "util.h"

_assetkit_hide
AkMesh*
ak_allocMesh(AkHeap      * __restrict heap,
             AkLibrary   * __restrict memp,
             AkGeometry ** __restrict geomLink) {
  AkGeometry *geom;
  AkObject   *meshObj;
  AkMesh     *mesh;

  /* create geometries */
  geom              = ak_heap_calloc(heap, memp, sizeof(*geom));
  geom->materialMap = ak_map_new(ak_cmp_str);

  /* destroy heap with this object */
  ak_setAttachedHeap(geom, geom->materialMap->heap);

  meshObj     = ak_objAlloc(heap, geom, sizeof(AkMesh), AK_GEOMETRY_MESH, true);
  geom->gdata = meshObj;
  mesh        = ak_objGet(meshObj);
  mesh->geom  = geom;
  
  if (geomLink)
    *geomLink = geom;

  return mesh;
}

_assetkit_hide
void
wobj_fixIndices(AkMeshPrimitive * __restrict prim) {
  AkUInt      *it;
  AkUIntArray *indices;
  AkAccessor  *acc;
  AkInput     *inp;
  int          j, indexStride;
  size_t       i;

  indices     = prim->indices;
  it          = indices->items;
  indexStride = prim->indexStride;

  for (i = 0; i < indices->count; i += indexStride) {
    inp = prim->input;
    j   = 0;

    while (inp && j < indexStride) {
      acc = inp->accessor;
      
      if (it[i + j] > 0) {
        it[i + j]--;
      } else {
        it[i + j] = acc->count - it[i + j]; /* count - 1 == last */
      }

      j++;
      inp = inp->next;
    }
  }
}

_assetkit_hide
AkInput*
wobj_addInput(WOState         * __restrict wst,
              AkDataContext   * __restrict dctx,
              AkMeshPrimitive * __restrict prim,
              AkInputSemantic              sem,
              const char      * __restrict semRaw,
              AkComponentSize              compSize,
              AkTypeId                     type,
              uint32_t                     offset) {
  AkHeap     *heap;
  AkBuffer   *buff;
  AkAccessor *acc;
  AkInput    *inp;
  AkTypeDesc *typeDesc;
  int         nComponents;

  heap        = wst->heap;
  typeDesc    = ak_typeDesc(type);
  nComponents = (int)compSize;

  buff         = ak_heap_calloc(heap, wst->doc, sizeof(*buff));
  buff->data   = ak_heap_alloc(heap, buff, dctx->usedsize);
  buff->length = dctx->usedsize;
  ak_data_join(dctx, buff->data, 0, 0);
  
  flist_sp_insert(&wst->doc->lib.buffers, buff);
  
  acc                 = ak_heap_calloc(heap, wst->doc, sizeof(*acc));
  acc->buffer         = buff;
  acc->byteLength     = buff->length;
  acc->byteStride     = typeDesc->size * nComponents;
  acc->componentSize  = compSize;
  acc->componentType  = type;
  acc->componentBytes = typeDesc->size * nComponents;
  acc->componentCount = nComponents;
  acc->fillByteSize   = typeDesc->size * nComponents;
  acc->count          = (uint32_t)dctx->itemcount;

  inp                 = ak_heap_calloc(heap, prim, sizeof(*inp));
  inp->accessor       = acc;
  inp->semantic       = sem;
  inp->semanticRaw    = ak_heap_strdup(heap, inp, semRaw);
  inp->offset         = offset;

  inp->next   = prim->input;
  prim->input = inp;
  
  return inp;
}

_assetkit_hide
void
wobj_joinIndices(WOState * __restrict wst, AkMeshPrimitive * __restrict prim) {
  void    *it;
  size_t   stride, isz_v, isz_t, isz_n, count;
  uint32_t istride;

  isz_v   = wst->obj.dc_indv->itemsize;
  isz_t   = wst->obj.dc_indt->itemsize;
  isz_n   = wst->obj.dc_indn->itemsize;
  
  count   = wst->obj.dc_indv->itemcount;
  stride  = isz_v;
  istride = 1;

  if (wst->obj.dc_indn->itemcount > 0) {
    count  += wst->obj.dc_indn->itemcount;
    stride += isz_n;
    istride++;
  }
  
  if (wst->obj.dc_indt->itemcount > 0) {
    count  += wst->obj.dc_indt->itemcount;
    stride += isz_t;
    istride++;
  }
  
  prim->indices = ak_heap_calloc(wst->heap,
                                 prim,
                                 sizeof(*prim->indices)
                                 + wst->obj.dc_indv->usedsize
                                 + wst->obj.dc_indn->usedsize
                                 + wst->obj.dc_indt->usedsize);
  prim->indices->count = count;
  prim->indexStride    = istride;

  it = prim->indices->items;

  ak_data_join(wst->obj.dc_indv, it, 0,             stride);
  ak_data_join(wst->obj.dc_indn, it, isz_v,         stride);
  ak_data_join(wst->obj.dc_indt, it, isz_v + isz_n, stride);
}
