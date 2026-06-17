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

#include "assetkit_draco_bridge.h"

#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  define AK_DRACO_EXPORT __declspec(dllexport)
#else
#  define AK_DRACO_EXPORT __attribute__((visibility("default")))
#endif

static const char AK_DRACO_KEY_ATTRIBUTES[] = "attributes";
static const char AK_DRACO_KEY_BUFFER_VIEW[] = "bufferView";
static const char AK_DRACO_KEY_INDICES[]    = "indices";

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
ak_draco_fill_attribute(struct AkGLTFState       * __restrict gst,
                        const draco::Mesh        * __restrict mesh,
                        AkAccessor               * __restrict acc,
                        const draco::PointAttribute * __restrict att) {
  AkBuffer *buff;
  AkDoc    *doc;
  AkHeap   *heap;
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

  heap = ak_draco_gltf_heap(gst);
  doc  = ak_draco_gltf_doc(gst);
  if (!heap || !doc)
    return false;

  stride = compSize * acc->componentCount;
  len    = stride * acc->count;
  buff   = (AkBuffer *)ak_heap_calloc(heap, doc, sizeof(*buff));

  buff->data   = ak_heap_alloc(heap, buff, len);
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

  ak_draco_gltf_prepend_buffer(gst, buff);

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
ak_draco_fill_indices(struct AkGLTFState * __restrict gst,
                      const draco::Mesh * __restrict mesh,
                      AkAccessor        * __restrict acc) {
  AkBuffer *buff;
  AkDoc    *doc;
  AkHeap   *heap;
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

  heap = ak_draco_gltf_heap(gst);
  doc  = ak_draco_gltf_doc(gst);
  if (!heap || !doc)
    return false;

  len          = compSize * idxCount;
  buff         = (AkBuffer *)ak_heap_calloc(heap, doc, sizeof(*buff));
  buff->data   = ak_heap_alloc(heap, buff, len);
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

  ak_draco_gltf_prepend_buffer(gst, buff);

  return true;
}

static
bool
ak_draco_fill_primitive(struct AkGLTFState * __restrict gst,
                        AkMeshPrimitive    * __restrict prim,
                        const struct json_t * __restrict jprim,
                        const struct json_t * __restrict jdraco,
                        const draco::Mesh * __restrict mesh) {
  const struct json_t *jattrs;
  const struct json_t *jdattrs;
  const struct json_t *jattr;
  const struct json_t *jdattr;
  const struct json_t *jidx;
  AkAccessor          *acc;
  const char          *key;
  size_t               keysize;
  int32_t              accIdx;
  int32_t              attId;

  if (!gst || !prim || !jprim || !jdraco || !mesh)
    return false;

  jidx = ak_draco_json_get(jprim, AK_DRACO_KEY_INDICES);
  if (jidx) {
    accIdx = ak_draco_json_int32(jidx, -1);
    acc    = ak_draco_gltf_accessor_at(gst, accIdx);
    if (!acc || !ak_draco_fill_indices(gst, mesh, acc))
      return false;
  }

  jattrs  = ak_draco_json_get(jprim,   AK_DRACO_KEY_ATTRIBUTES);
  jdattrs = ak_draco_json_get(jdraco,  AK_DRACO_KEY_ATTRIBUTES);
  if (!jattrs || !jdattrs)
    return false;

  jattr = ak_draco_json_first_child(jattrs);
  while (jattr) {
    key     = ak_draco_json_key(jattr);
    keysize = ak_draco_json_keysize(jattr);
    jdattr  = ak_draco_json_get_len(jdattrs, key, keysize);
    if (!jdattr)
      return false;

    accIdx = ak_draco_json_int32(jattr,  -1);
    attId  = ak_draco_json_int32(jdattr, -1);
    acc    = ak_draco_gltf_accessor_at(gst, accIdx);
    if (!acc)
      return false;

    if (!ak_draco_fill_attribute(gst,
                                 mesh,
                                 acc,
                                 mesh->GetAttributeByUniqueId((uint32_t)attId)))
      return false;

    jattr = ak_draco_json_next(jattr);
  }

  return true;
}

extern "C"
AK_DRACO_EXPORT
int
ak_draco_decode_gltf_primitive(struct AkGLTFState *gst,
                               AkMeshPrimitive    *prim,
                               const struct json_t *jprim,
                               const struct json_t *jdraco) {
  AkDracoBufferView bv;
  AkBuffer         *buff;
  const struct json_t *it;
  const char       *src;
  size_t            off;
  int32_t           bvIdx;

  draco::Decoder       dec;
  draco::DecoderBuffer dbuf;
  std::unique_ptr<draco::Mesh> mesh;

  if (!gst || !prim || !jprim || !jdraco)
    return -1;

  it    = ak_draco_json_get(jdraco, AK_DRACO_KEY_BUFFER_VIEW);
  bvIdx = it ? ak_draco_json_int32(it, -1) : -1;
  if (bvIdx < 0)
    return -1;

  if (!ak_draco_gltf_buffer_view_at(gst, bvIdx, &bv)
      || !(buff = bv.buffer)
      || !buff->data)
    return -1;

  if (bv.byteOffset > buff->length
      || bv.byteLength == 0
      || bv.byteLength > buff->length - bv.byteOffset)
    return -1;

  off = bv.byteOffset;
  src = (const char *)buff->data + off;
  dbuf.Init(src, bv.byteLength);

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
