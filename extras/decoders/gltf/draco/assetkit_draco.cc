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

#include <draco/compression/decode.h>

#include <memory>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  define AK_DRACO_EXPORT __declspec(dllexport)
#else
#  define AK_DRACO_EXPORT __attribute__((visibility("default")))
#endif

typedef struct AkHeap AkHeap;
typedef struct AkMeshPrimitive AkMeshPrimitive;
typedef struct FListItem FListItem;
typedef struct RBTree RBTree;

typedef enum AkTypeId {
  AKT_NONE   = 0,
  AKT_INT    = 6,
  AKT_FLOAT  = 10,
  AKT_BYTE   = 29,
  AKT_UBYTE  = 30,
  AKT_SHORT  = 31,
  AKT_USHORT = 32,
  AKT_UINT   = 28
} AkTypeId;

typedef enum json_type {
  JSON_OBJECT = 1,
  JSON_ARRAY  = 2,
  JSON_STRING = 3
} json_type_t;

typedef struct json_t {
  struct json_t *parent;
  struct json_t *next;
  const char    *key;
  void          *value;
  int32_t        valsize;
  int32_t        keysize;
  json_type_t    type;
} json_t;

typedef struct AkBuffer {
  const char *name;
  void       *data;
  size_t      length;
} AkBuffer;

typedef struct AkAccessor {
  AkBuffer *buffer;
  const char *name;
  void       *min;
  void       *max;
  size_t      byteOffset;
  size_t      byteStride;
  size_t      byteLength;
  uint32_t    count;
  uint32_t    bytesPerComponent;
  int32_t     componentSize;
  AkTypeId    componentType;
  uint32_t    componentCount;
  size_t      fillByteSize;
  int32_t     gpuTarget;
  bool        normalized;
  AkTypeId    originalComponentType;
  bool        originallyNormalized;
} AkAccessor;

typedef struct AkLibrary {
  struct AkLibrary *next;
  const char       *name;
  void             *extra;
  void             *chld;
  uint64_t          count;
} AkLibrary;

typedef struct AkLibraries {
  AkLibrary *cameras;
  AkLibrary *lights;
  AkLibrary *effects;
  AkLibrary *libimages;
  AkLibrary *materials;
  AkLibrary *geometries;
  AkLibrary *controllers;
  AkLibrary *visualScenes;
  AkLibrary *nodes;
  AkLibrary *animations;
  FListItem *buffers;
  FListItem *accessors;
  FListItem *textures;
  FListItem *samplers;
  FListItem *images;
  void      *morphs;
  void      *skins;
} AkLibraries;

typedef struct AkDoc {
  void        *inf;
  void        *coordSys;
  void        *unit;
  void        *extra;
  void        *reserved;
  void        *userData;
  float        loadMillis;
  AkLibraries  lib;
} AkDoc;

typedef struct AkBufferView {
  AkBuffer   *buffer;
  const char *name;
  size_t      byteOffset;
  size_t      byteLength;
  size_t      byteStride;
} AkBufferView;

typedef struct AkGLTFState {
  AkHeap       *heap;
  AkDoc        *doc;
  json_t       *root;
  void         *tmpParent;
  FListItem    *buffers;
  RBTree       *bufferMap;
  FListItem    *bufferViews;
  RBTree       *skinBound;
  RBTree       *meshTargets;
  void         *bindata;
  void         *defaultMaterial;
  void         *meshopt;
  void         *draco;
  size_t        bindataLen;
  bool          stop;
  bool          isbinary;
} AkGLTFState;

extern "C" {
void *ak_heap_alloc(AkHeap *heap, void *parent, size_t size);
void *ak_heap_calloc(AkHeap *heap, void *parent, size_t size);
void  flist_sp_insert(FListItem **first, void *value);
void *flist_sp_at(FListItem **first, int32_t index);
}

extern "C" {
#include "io/gltf/strpool.h"
}

static
int32_t
ak_draco_json_int32(const json_t * __restrict obj, int32_t def) {
  char *end;
  long  val;

  if (!obj || obj->type != JSON_STRING || !obj->value)
    return def;

  errno = 0;
  val   = strtol((const char *)obj->value, &end, 10);
  if (errno != 0 || end == (const char *)obj->value)
    return def;

  return (int32_t)val;
}

static
json_t*
ak_draco_json_get(const json_t * __restrict object,
                  const char   * __restrict key) {
  json_t *it;
  size_t  keysize;

  if (!object || object->type != JSON_OBJECT || !key)
    return NULL;

  keysize = strlen(key);
  it      = (json_t *)object->value;
  while (it
         && ((size_t)it->keysize != keysize
             || strncmp(it->key, key, keysize) != 0))
    it = it->next;

  return it;
}

#define json_get   ak_draco_json_get
#define json_int32 ak_draco_json_int32

static
json_t*
ak_draco_json_getn(const json_t * __restrict object,
                   const char   * __restrict key,
                   size_t                    keysize) {
  json_t *it;

  if (!object || object->type != JSON_OBJECT || !key)
    return NULL;

  it = (json_t *)object->value;
  while (it
         && ((size_t)it->keysize != keysize
             || strncmp(it->key, key, keysize) != 0))
    it = it->next;

  return it;
}

static
size_t
ak_draco_component_size(AkTypeId type) {
  switch (type) {
    case AKT_BYTE:
    case AKT_UBYTE:
      return 1;
    case AKT_SHORT:
    case AKT_USHORT:
      return 2;
    case AKT_INT:
    case AKT_UINT:
    case AKT_FLOAT:
      return 4;
    default:
      break;
  }

  return 0;
}

static
bool
ak_draco_store_value(const draco::PointAttribute * __restrict att,
                     draco::PointIndex                       pidx,
                     AkAccessor                 * __restrict acc,
                     char                       * __restrict dst) {
  draco::AttributeValueIndex avi;
  int8_t                     comps;

  if (!att || !acc || !dst)
    return false;

  avi   = att->mapped_index(pidx);
  comps = (int8_t)acc->componentCount;

  switch (acc->componentType) {
    case AKT_BYTE:
      return att->ConvertValue<int8_t>(avi, comps, (int8_t *)dst);
    case AKT_UBYTE:
      return att->ConvertValue<uint8_t>(avi, comps, (uint8_t *)dst);
    case AKT_SHORT:
      return att->ConvertValue<int16_t>(avi, comps, (int16_t *)dst);
    case AKT_USHORT:
      return att->ConvertValue<uint16_t>(avi, comps, (uint16_t *)dst);
    case AKT_INT:
      return att->ConvertValue<int32_t>(avi, comps, (int32_t *)dst);
    case AKT_UINT:
      return att->ConvertValue<uint32_t>(avi, comps, (uint32_t *)dst);
    case AKT_FLOAT:
      return att->ConvertValue<float>(avi, comps, (float *)dst);
    default:
      break;
  }

  return false;
}

static
bool
ak_draco_fill_attribute(AkGLTFState              * __restrict gst,
                        const draco::Mesh        * __restrict mesh,
                        AkAccessor               * __restrict acc,
                        const draco::PointAttribute * __restrict att) {
  AkBuffer *buff;
  char     *dst;
  size_t    compSize;
  size_t    stride;
  size_t    len;
  uint32_t  i;

  if (!gst || !mesh || !acc || !att)
    return false;

  if (acc->count != (uint32_t)mesh->num_points())
    return false;

  compSize = ak_draco_component_size(acc->componentType);
  if (compSize == 0 || acc->componentCount == 0)
    return false;

  stride = compSize * acc->componentCount;
  len    = stride * acc->count;
  buff   = (AkBuffer *)ak_heap_calloc(gst->heap, gst->doc, sizeof(*buff));

  buff->data   = ak_heap_alloc(gst->heap, buff, len);
  buff->length = len;
  dst          = (char *)buff->data;

  for (i = 0; i < acc->count; i++) {
    if (!ak_draco_store_value(att,
                              draco::PointIndex(i),
                              acc,
                              dst + (size_t)i * stride))
      return false;
  }

  acc->buffer            = buff;
  acc->byteOffset        = 0;
  acc->bytesPerComponent = (uint32_t)compSize;
  acc->fillByteSize      = stride;
  acc->byteStride        = stride;
  acc->byteLength        = len;

  flist_sp_insert(&gst->doc->lib.buffers, buff);

  return true;
}

static
bool
ak_draco_write_index(char     * __restrict dst,
                     AkTypeId              type,
                     uint32_t              val) {
  switch (type) {
    case AKT_UBYTE:
      if (val > UINT8_MAX) return false;
      *(uint8_t *)dst = (uint8_t)val;
      return true;
    case AKT_USHORT:
      if (val > UINT16_MAX) return false;
      *(uint16_t *)dst = (uint16_t)val;
      return true;
    case AKT_UINT:
      *(uint32_t *)dst = val;
      return true;
    default:
      break;
  }

  return false;
}

static
bool
ak_draco_fill_indices(AkGLTFState       * __restrict gst,
                      const draco::Mesh * __restrict mesh,
                      AkAccessor        * __restrict acc) {
  AkBuffer *buff;
  char     *dst;
  size_t    compSize;
  size_t    len;
  uint32_t  faceCount;
  uint32_t  idxCount;
  uint32_t  f;
  uint32_t  c;
  uint32_t  outIdx;
  const draco::Mesh::Face *face;

  if (!gst || !mesh || !acc)
    return false;

  faceCount = (uint32_t)mesh->num_faces();
  idxCount  = faceCount * 3;

  if (acc->count != idxCount)
    return false;

  compSize = ak_draco_component_size(acc->componentType);
  if (compSize == 0)
    return false;

  len          = compSize * idxCount;
  buff         = (AkBuffer *)ak_heap_calloc(gst->heap, gst->doc, sizeof(*buff));
  buff->data   = ak_heap_alloc(gst->heap, buff, len);
  buff->length = len;
  dst          = (char *)buff->data;
  outIdx       = 0;
  face         = NULL;

  for (f = 0; f < faceCount; f++) {
    face = &mesh->face(draco::FaceIndex(f));

    for (c = 0; c < 3; c++, outIdx++) {
      if (!ak_draco_write_index(dst + (size_t)outIdx * compSize,
                                acc->componentType,
                                (uint32_t)(*face)[c].value()))
        return false;
    }
  }

  acc->buffer            = buff;
  acc->byteOffset        = 0;
  acc->bytesPerComponent = (uint32_t)compSize;
  acc->componentCount    = 1;
  acc->fillByteSize      = compSize;
  acc->byteStride        = compSize;
  acc->byteLength        = len;

  flist_sp_insert(&gst->doc->lib.buffers, buff);

  return true;
}

static
bool
ak_draco_fill_primitive(AkGLTFState       * __restrict gst,
                        AkMeshPrimitive  * __restrict prim,
                        const json_t     * __restrict jprim,
                        const json_t     * __restrict jdraco,
                        const draco::Mesh * __restrict mesh) {
  const json_t *jattrs;
  const json_t *jdattrs;
  const json_t *jattr;
  const json_t *jdattr;
  const json_t *jidx;
  AkAccessor   *acc;
  int32_t       accIdx;
  int32_t       attId;

  if (!gst || !prim || !jprim || !jdraco || !mesh)
    return false;

  jidx = json_get(jprim, _s_gltf_indices);
  if (jidx) {
    accIdx = json_int32(jidx, -1);
    acc    = (AkAccessor *)flist_sp_at(&gst->doc->lib.accessors, accIdx);
    if (!acc || !ak_draco_fill_indices(gst, mesh, acc))
      return false;
  }

  jattrs  = json_get(jprim,   _s_gltf_attributes);
  jdattrs = json_get(jdraco,  _s_gltf_attributes);
  if (!jattrs || !jdattrs)
    return false;

  jattr = (json_t *)jattrs->value;
  while (jattr) {
    jdattr = ak_draco_json_getn(jdattrs, jattr->key, (size_t)jattr->keysize);
    if (!jdattr)
      return false;

    accIdx = json_int32(jattr,  -1);
    attId  = json_int32(jdattr, -1);
    acc    = (AkAccessor *)flist_sp_at(&gst->doc->lib.accessors, accIdx);
    if (!acc)
      return false;

    if (!ak_draco_fill_attribute(gst,
                                 mesh,
                                 acc,
                                 mesh->GetAttributeByUniqueId((uint32_t)attId)))
      return false;

    jattr = jattr->next;
  }

  return true;
}

extern "C"
AK_DRACO_EXPORT
int
ak_draco_decode_gltf_primitive(AkGLTFState     *gst,
                               AkMeshPrimitive *prim,
                               const json_t    *jprim,
                               const json_t    *jdraco) {
  AkBufferView *bv;
  AkBuffer     *buff;
  const json_t *it;
  const char   *src;
  size_t        off;
  int32_t       bvIdx;

  draco::Decoder       dec;
  draco::DecoderBuffer dbuf;
  std::unique_ptr<draco::Mesh> mesh;

  if (!gst || !prim || !jprim || !jdraco)
    return -1;

  it    = json_get(jdraco, _s_gltf_bufferView);
  bvIdx = it ? json_int32(it, -1) : -1;
  if (bvIdx < 0)
    return -1;

  bv = (AkBufferView *)flist_sp_at(&gst->bufferViews, bvIdx);
  if (!bv || !(buff = bv->buffer) || !buff->data)
    return -1;

  if (bv->byteOffset > buff->length
      || bv->byteLength == 0
      || bv->byteLength > buff->length - bv->byteOffset)
    return -1;

  off = bv->byteOffset;
  src = (const char *)buff->data + off;
  dbuf.Init(src, bv->byteLength);

  {
    draco::StatusOr<std::unique_ptr<draco::Mesh> > meshRes =
      dec.DecodeMeshFromBuffer(&dbuf);

    if (!meshRes.ok())
      return -1;

    mesh = std::move(meshRes).value();
  }
  if (!mesh)
    return -1;

  return ak_draco_fill_primitive(gst, prim, jprim, jdraco, mesh.get()) ? 0 : -1;
}
