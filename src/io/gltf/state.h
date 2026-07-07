/*
 * Copyright (C) 2026 Recep Aslantas
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

#ifndef gltf_state_h
#define gltf_state_h

#include "../../../include/ak/assetkit.h"
#include "strpool.h"
#include "../../string_fast.h"

#include <ds/forward-list-sep.h>
#include <ds/rb.h>
#include <json/common.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct AkBufferView {
  AkBuffer   *buffer;
  const char *name;
  size_t      byteOffset;
  size_t      byteLength;
  size_t      byteStride;
} AkBufferView;

typedef struct AkGLTFMeshoptLib AkGLTFMeshoptLib;
typedef struct AkGLTFDracoLib   AkGLTFDracoLib;
typedef struct AkGLTFSPZLib     AkGLTFSPZLib;
typedef struct AkGLTFKTX2Lib    AkGLTFKTX2Lib;

typedef struct AkGLTFState {
  AkHeap       *heap;
  AkDoc        *doc;
  json_t       *root;
  void         *tmpParent;
  FListItem    *buffers;
  AkBuffer    **buffersByIndex;
  RBTree       *bufferMap;
  AkBufferView **bufferViewsByIndex;
  AkAccessor  **accessorsByIndex;
  AkImage     **imagesByIndex;
  AkSampler   **samplersByIndex;
  AkTexture   **texturesByIndex;
  AkMaterial  **materialsByIndex;
  AkGeometry  **geometriesByIndex;
  AkCamera    **camerasByIndex;
  AkNode      **nodesByIndex;
  RBTree       *skinBound;
  RBTree       *meshTargets;
  void         *bindata;
  void         *defaultMaterial;
  AkGLTFMeshoptLib *meshopt;
  AkGLTFDracoLib   *draco;
  AkGLTFSPZLib     *spz;   /* Gaussian splat (SPZ) decoder, optional */
  AkGLTFKTX2Lib    *ktx2;  /* KTX2/BasisU decoder, optional */
  AkSampler        *defaultSampler;
  size_t        bindataLen;
  size_t        buffersCount;
  size_t        bufferViewsCount;
  size_t        accessorsCount;
  size_t        imagesCount;
  size_t        samplersCount;
  size_t        texturesCount;
  size_t        materialsCount;
  size_t        geometriesCount;
  size_t        camerasCount;
  size_t        nodesCount;
  bool          stop;
  bool          isbinary;
  bool          animPointerRequired;
  bool          borrowBufferViews;
} AkGLTFState;

#define GLTF_INDEXED_AT(ARR, COUNT, INDEX)                                    \
  (((INDEX) >= 0 && (size_t)(INDEX) < (COUNT)) ? (ARR)[(size_t)(INDEX)] : NULL)

#define AK_GLTF_LIB_PREPEND(LIB, ITEM, NEXT)                                  \
  do {                                                                        \
    (ITEM)->NEXT = (LIB).first;                                               \
    if (!(LIB).last)                                                          \
      (LIB).last = (ITEM);                                                    \
    (LIB).first = (ITEM);                                                     \
    (LIB).count++;                                                            \
  } while (0)

static inline
bool
gltf_jsonKeyEqLen(const json_t * __restrict obj,
                  const char   * __restrict key,
                  size_t                    len) {
  return obj
      && obj->key
      && (size_t)obj->keysize == len
      && ak_str_eq_fast(obj->key, (size_t)obj->keysize, key, len);
}

static inline
json_t*
gltf_jsonGetLen(const json_t * __restrict object,
                const char   * __restrict key,
                size_t                    len) {
  json_t *iter;

  if (!object
      || object->type != JSON_OBJECT
      || !key
      || !(iter = (json_t *)object->value))
    return NULL;

  while (iter && !gltf_jsonKeyEqLen(iter, key, len))
    iter = iter->next;

  return iter;
}

static inline
json_t*
gltf_jsonGet(const json_t * __restrict object,
             const char   * __restrict key) {
  return key ? gltf_jsonGetLen(object, key, strlen(key)) : NULL;
}

static inline
int32_t
gltf_jsonInt32(const json_t * __restrict obj, int32_t def) {
  const char *p;
  const char *end;
  AkInt       val;

  if (!obj || obj->type != JSON_STRING || !obj->value)
    return def;

  p   = (const char *)obj->value;
  end = p + obj->valsize;
  if (p < end && (*p == '-' || *p == '+'))
    p++;
  if (p >= end || *p < '0' || *p > '9')
    return def;

  ak_str_parse_int_fast((char *)obj->value, (char *)end, &val);
  return (int32_t)val;
}

static inline
AkBuffer*
gltf_buffer_at(AkGLTFState * __restrict gst, int32_t index) {
  AkBuffer *item;

  if ((item = GLTF_INDEXED_AT(gst->buffersByIndex, gst->buffersCount, index)))
    return item;

  return (AkBuffer *)flist_sp_at(&gst->buffers, index);
}

static inline
AkBufferView*
gltf_bufferView_at(AkGLTFState * __restrict gst, int32_t index) {
  return GLTF_INDEXED_AT(gst->bufferViewsByIndex, gst->bufferViewsCount, index);
}

static inline
AkAccessor*
gltf_accessor_at(AkGLTFState * __restrict gst, int32_t index) {
  AkAccessor *item;
  int32_t     i;

  if ((item = GLTF_INDEXED_AT(gst->accessorsByIndex, gst->accessorsCount, index)))
    return item;

  item = gst->doc->lib.accessors.first;
  for (i = 0; item && i < index; i++)
    item = item->next;

  return item;
}

static inline
AkImage*
gltf_image_at(AkGLTFState * __restrict gst, int32_t index) {
  AkImage *item;
  int32_t  i;

  if ((item = GLTF_INDEXED_AT(gst->imagesByIndex, gst->imagesCount, index)))
    return item;

  item = gst->doc->lib.images.first;
  for (i = 0; item && i < index; i++)
    item = item->next;

  return item;
}

static inline
AkSampler*
gltf_sampler_at(AkGLTFState * __restrict gst, int32_t index) {
  AkSampler *item;
  int32_t    i;

  if ((item = GLTF_INDEXED_AT(gst->samplersByIndex, gst->samplersCount, index)))
    return item;

  item = gst->doc->lib.samplers.first;
  for (i = 0; item && i < index; i++)
    item = item->next;

  return item;
}

static inline
AkTexture*
gltf_texture_at(AkGLTFState * __restrict gst, int32_t index) {
  AkTexture *item;
  int32_t    i;

  if ((item = GLTF_INDEXED_AT(gst->texturesByIndex, gst->texturesCount, index)))
    return item;

  item = gst->doc->lib.textures.first;
  for (i = 0; item && i < index; i++)
    item = item->next;

  return item;
}

static inline
AkMaterial*
gltf_material_at(AkGLTFState * __restrict gst, int32_t index) {
  AkMaterial *item;
  int32_t     i;

  if ((item = GLTF_INDEXED_AT(gst->materialsByIndex, gst->materialsCount, index)))
    return item;

  item = gst->doc->lib.materials.first;
  for (i = 0; item && i < index; i++)
    item = item->next;

  return item;
}

static inline
AkGeometry*
gltf_geometry_at(AkGLTFState * __restrict gst, int32_t index) {
  AkGeometry *item;
  int32_t     i;

  if ((item = GLTF_INDEXED_AT(gst->geometriesByIndex, gst->geometriesCount, index)))
    return item;

  item = gst->doc->lib.geometries.first;
  for (i = 0; item && i < index; i++)
    item = item->next;

  return item;
}

static inline
AkCamera*
gltf_camera_at(AkGLTFState * __restrict gst, int32_t index) {
  AkCamera *item;
  int32_t   i;

  if ((item = GLTF_INDEXED_AT(gst->camerasByIndex, gst->camerasCount, index)))
    return item;

  item = gst->doc->lib.cameras.first;
  for (i = 0; item && i < index; i++)
    item = item->next;

  return item;
}

static inline
AkNode*
gltf_node_at(AkGLTFState * __restrict gst, int32_t index) {
  return GLTF_INDEXED_AT(gst->nodesByIndex, gst->nodesCount, index);
}

#endif /* gltf_state_h */
