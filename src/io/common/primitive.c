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

#include "primitive.h"

#include <string.h>

AK_HIDE
uint32_t
io_primitive_vertex_count(AkMeshPrimitive * __restrict prim) {
  AkAccessor *idxAcc;

  if (!prim)
    return 0;

  if (prim->indices) {
    uint32_t stride;

    stride = prim->indexStride ? prim->indexStride : 1u;
    return (uint32_t)(prim->indices->count / stride);
  }

  idxAcc = prim->indexAccessor;
  if (idxAcc)
    return idxAcc->count;

  if (prim->pos && prim->pos->accessor)
    return prim->pos->accessor->count;

  return 0;
}

static
AkUInt
io_index_accessor_get(AkAccessor * __restrict acc, uint32_t index) {
  const unsigned char *src;
  size_t              stride;

  if (!acc || !acc->buffer || !acc->buffer->data || index >= acc->count)
    return index;

  stride = acc->byteStride ? acc->byteStride : acc->bytesPerComponent;
  src    = (const unsigned char *)acc->buffer->data
           + acc->byteOffset
           + (size_t)index * stride;

  switch (acc->componentType) {
    case AKT_UBYTE:
      return src[0];
    case AKT_USHORT: {
      uint16_t v;
      memcpy(&v, src, sizeof(v));
      return v;
    }
    case AKT_UINT: {
      uint32_t v;
      memcpy(&v, src, sizeof(v));
      return v;
    }
    default:
      return index;
  }
}

AK_HIDE
AkUInt
io_primitive_input_index(AkMeshPrimitive * __restrict prim,
                         AkInput         * __restrict input,
                         uint32_t                     vertexIndex) {
  if (prim->indices) {
    uint32_t stride;
    uint32_t offset;

    stride = prim->indexStride ? prim->indexStride : 1u;
    offset = input ? input->indexOffset : 0u;
    if (offset >= stride)
      offset = 0;
    return ak_indexArrayGet(prim->indices,
                            (size_t)vertexIndex * stride + offset);
  }

  if (prim->indexAccessor)
    return io_index_accessor_get(prim->indexAccessor, vertexIndex);

  return vertexIndex;
}

AK_HIDE
bool
io_accessor_float_direct(AkAccessor * __restrict acc) {
  size_t fillSize;
  size_t stride;

  if (!acc
      || !acc->buffer
      || !acc->buffer->data
      || acc->componentType != AKT_FLOAT
      || acc->bytesPerComponent != sizeof(float)
      || acc->componentCount == 0
      || acc->count == 0)
    return false;

  fillSize = acc->fillByteSize
             ? acc->fillByteSize
             : (size_t)acc->bytesPerComponent * acc->componentCount;
  stride   = acc->byteStride ? acc->byteStride : fillSize;

  if (fillSize < sizeof(float) * acc->componentCount
      || stride < fillSize
      || acc->byteOffset > acc->buffer->length
      || (size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;

  return acc->byteOffset + (size_t)(acc->count - 1u) * stride + fillSize
         <= acc->buffer->length;
}

AK_HIDE
const float*
io_accessor_float_row(AkAccessor * __restrict acc, uint32_t index) {
  size_t fillSize;
  size_t stride;

  fillSize = acc->fillByteSize
             ? acc->fillByteSize
             : (size_t)acc->bytesPerComponent * acc->componentCount;
  stride   = acc->byteStride ? acc->byteStride : fillSize;

  return (const float *)((const char *)acc->buffer->data
                         + acc->byteOffset
                         + (size_t)index * stride);
}
