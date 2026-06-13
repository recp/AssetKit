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

#include <stdlib.h>
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

AK_HIDE
bool
io_float_rows_init(IOFloatRows * __restrict rows,
                   AkAccessor  * __restrict acc) {
  size_t fillSize;
  size_t floatCount;

  memset(rows, 0, sizeof(*rows));
  if (!acc || acc->count == 0 || acc->componentCount == 0)
    return false;

  rows->accessor       = acc;
  rows->componentCount = acc->componentCount;
  rows->direct         = io_accessor_float_direct(acc);
  if (rows->direct) {
    fillSize = acc->fillByteSize
               ? acc->fillByteSize
               : (size_t)acc->bytesPerComponent * acc->componentCount;
    rows->byteStride = acc->byteStride ? acc->byteStride : fillSize;
    rows->directData = (const char *)acc->buffer->data + acc->byteOffset;
    return true;
  }

  if ((size_t)acc->count > (size_t)-1 / acc->componentCount)
    return false;

  floatCount       = (size_t)acc->count * acc->componentCount;
  rows->byteStride = sizeof(float) * acc->componentCount;
  rows->scratch    = malloc(sizeof(float) * floatCount);
  if (!rows->scratch)
    return false;

  if (ak_accessorAsFloat(acc, rows->scratch, floatCount) != floatCount) {
    free(rows->scratch);
    rows->scratch = NULL;
    return false;
  }

  return true;
}

AK_HIDE
void
io_float_rows_destroy(IOFloatRows * __restrict rows) {
  free(rows->scratch);
  rows->scratch = NULL;
}

AK_HIDE
bool
io_triangle_iter_init(IOTriangleIter  * __restrict it,
                      AkMeshPrimitive * __restrict prim) {
  AkTriangleMode mode;
  uint32_t       count;

  memset(it, 0, sizeof(*it));
  if (!prim || prim->type != AK_PRIMITIVE_TRIANGLES)
    return false;

  count = io_primitive_vertex_count(prim);
  if (count < 3u)
    return false;

  mode = ((AkTriangles *)prim)->mode;
  if (mode == 0)
    mode = AK_TRIANGLES;

  it->mode   = mode;
  it->count  = count;
  it->cursor = mode == AK_TRIANGLE_FAN ? 1u : 0u;
  return true;
}

AK_HIDE
bool
io_triangle_iter_next(IOTriangleIter * __restrict it,
                      uint32_t                    tri[3]) {
  uint32_t i;

  i = it->cursor;
  switch (it->mode) {
    case AK_TRIANGLE_STRIP:
      if (i + 2u >= it->count)
        return false;
      if (i & 1u) {
        tri[0] = i + 1u;
        tri[1] = i;
      } else {
        tri[0] = i;
        tri[1] = i + 1u;
      }
      tri[2] = i + 2u;
      it->cursor = i + 1u;
      return true;

    case AK_TRIANGLE_FAN:
      if (i + 1u >= it->count)
        return false;
      tri[0] = 0u;
      tri[1] = i;
      tri[2] = i + 1u;
      it->cursor = i + 1u;
      return true;

    case AK_TRIANGLES:
    default:
      if (i + 2u >= it->count)
        return false;
      tri[0] = i;
      tri[1] = i + 1u;
      tri[2] = i + 2u;
      it->cursor = i + 3u;
      return true;
  }
}
