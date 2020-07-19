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

AK_HIDE
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

AK_HIDE
AkInput*
io_addInput(AkHeap          * __restrict heap,
            AkDataContext   * __restrict dctx,
            AkMeshPrimitive * __restrict prim,
            AkInputSemantic              sem,
            const char      * __restrict semRaw,
            AkComponentSize              compSize,
            AkTypeId                     type,
            uint32_t                     offset) {
  AkDoc      *doc;
  AkBuffer   *buff;
  AkAccessor *acc;
  AkInput    *inp;
  AkTypeDesc *typeDesc;
  int         nComponents;

  doc         = ak_heap_data(heap);
  typeDesc    = ak_typeDesc(type);
  nComponents = (int)compSize;

  buff         = ak_heap_calloc(heap, doc, sizeof(*buff));
  buff->data   = ak_heap_alloc(heap, buff, dctx->usedsize);
  buff->length = dctx->usedsize;
  ak_data_join(dctx, buff->data, 0, 0);
  
  flist_sp_insert(&doc->lib.buffers, buff);
  
  acc                 = ak_heap_calloc(heap, doc, sizeof(*acc));
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
  prim->inputCount++;
  
  return inp;
}
