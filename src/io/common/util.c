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
bool
io_rb_reserve_key(RBTree * __restrict map,
                  void   * __restrict key) {
  if (!key)
    return false;

  if (rb_find(map, key))
    return true;

  rb_insert(map, key, (void *)(uintptr_t)1);

  return true;
}

AK_HIDE
bool
io_rb_insert_absent_key(RBTree * __restrict map,
                        void   * __restrict key) {
  if (!key || rb_find(map, key))
    return false;

  rb_insert(map, key, (void *)(uintptr_t)1);

  return true;
}

AK_HIDE
AkMesh*
ak_allocMeshEx(AkHeap      * __restrict heap,
               void        * __restrict memp,
               AkGeometry ** __restrict geomLink,
               bool                      materialMap) {
  AkGeometry *geom;
  AkObject   *meshObj;
  AkMesh     *mesh;

  /* create geometries */
  geom = ak_heap_calloc(heap, memp, sizeof(*geom));
  if (materialMap) {
    geom->materialMap = ak_map_new(ak_cmp_str);

    /* destroy heap with this object */
    ak_setAttachedHeap(geom, geom->materialMap->heap);
  }

  meshObj     = ak_objAlloc(heap, geom, sizeof(AkMesh), AK_GEOMETRY_MESH, true);
  geom->gdata = meshObj;
  mesh        = ak_objGet(meshObj);
  mesh->geom  = geom;
  
  if (geomLink)
    *geomLink = geom;

  return mesh;
}

AK_HIDE
AkMesh*
ak_allocMesh(AkHeap      * __restrict heap,
             void        * __restrict memp,
             AkGeometry ** __restrict geomLink) {
  return ak_allocMeshEx(heap, memp, geomLink, true);
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

  doc          = ak_heap_data(heap);

  buff         = ak_heap_calloc(heap, doc, sizeof(*buff));
  buff->data   = ak_heap_alloc(heap, buff, dctx->usedsize);
  buff->length = dctx->usedsize;
  ak_data_join(dctx, buff->data, 0, 0);
  
  AK_LIB_PREPEND(doc->lib.buffers, buff, next);
  
  acc = io_acc(heap, doc, compSize, type, (uint32_t)dctx->itemcount, buff);
  AK_LIB_PREPEND(doc->lib.accessors, acc, next);

  inp = io_input(heap, prim, acc, sem, semRaw, offset);
  ak_retain(acc);

  return inp;
}

AK_HIDE
AkAccessor*
io_acc(AkHeap          * __restrict heap,
       AkDoc           * __restrict doc,
       AkComponentSize              compSize,
       AkTypeId                     type,
       uint32_t                     count,
       AkBuffer        * __restrict buff) {
  AkAccessor *acc;
  AkTypeDesc *typeDesc;
  int         nComponents;

  typeDesc    = ak_typeDesc(type);
  nComponents = (int)compSize;
  
  acc                    = ak_heap_calloc(heap, doc, sizeof(*acc));
  acc->buffer            = buff;
  acc->byteLength        = buff->length;
  acc->byteStride        = typeDesc->size * nComponents;
  acc->componentSize     = compSize;
  acc->componentType          = type;
  acc->originalComponentType  = type;
  acc->bytesPerComponent      = typeDesc->size;
  acc->componentCount         = nComponents;
  acc->fillByteSize           = typeDesc->size * nComponents;
  acc->count                  = count;

  return acc;
}

AK_HIDE
AkInput*
io_input(AkHeap          * __restrict heap,
         AkMeshPrimitive * __restrict prim,
         AkAccessor      * __restrict acc,
         AkInputSemantic              sem,
         const char      * __restrict semRaw,
         uint32_t                     offset) {
  AkInput *inp;

  inp              = ak_heap_calloc(heap, prim, sizeof(*inp));
  inp->accessor    = acc;
  inp->semantic    = sem;
  inp->semanticRaw = ak_heap_strdup(heap, inp, semRaw);
  inp->indexOffset = offset;

  inp->next   = prim->input;
  prim->input = inp;
  prim->inputCount++;
  
  return inp;
}
