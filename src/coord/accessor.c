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
#include "common.h"

AK_HIDE
bool
ak_coordBufferWritable(AkBuffer *buffer) {
  AkMemoryMapNode **maps, *map;
  AkHeapNode      *node;
  AkHeap          *heap;
  void            *parent, *copy;
  uintptr_t        address, start;

  if (!buffer || !buffer->data)
    return false;

  address = (uintptr_t)buffer->data;

  /* External glTF buffers attach their mapping to the buffer; a GLB attaches
     it to the document. Keep the mapping alive for other borrowed views. */
  for (parent = buffer; parent; parent = ak_mem_parent(parent)) {
    node = ak__alignof(parent);
    maps = ak_heap_ext_get(node, AK_HEAP_NODE_FLAGS_MMAP);

    if (!maps)
      continue;

    for (map = *maps; map; map = map->next) {
      start = (uintptr_t)map->mapped;

      if (address < start || address - start >= map->sized)
        continue;

      if (buffer->length > map->sized - (address - start)
          || !(heap = ak_heap_getheap(buffer))
          || !(copy = ak_heap_alloc(heap, buffer, buffer->length)))
        return false;

      memcpy(copy, buffer->data, buffer->length);
      buffer->data = copy;
      return true;
    }
  }

  return true;
}

static
bool
ak_coord_cvt_accessor_vector(AkAccessor * __restrict acc,
                              AkCoordSys * __restrict oldCoordSys,
                              AkCoordSys * __restrict newCoordSys,
                              bool                    noSign,
                              bool                    tangent) {
  unsigned char *data, *row;
  size_t         rowBytes, valueBytes, stride, last, offset[3];
  AkAxisAccessor a0, a1;
  uint32_t       i;
  float          values[3], sign[3], w;
  bool           flipW;

  if (!acc
      || !oldCoordSys
      || !newCoordSys
      || oldCoordSys == newCoordSys
      || acc->count == 0
      || acc->componentCount < 3)
    return true;

  if (acc->componentType != AKT_FLOAT
      || acc->normalized
      || acc->bytesPerComponent != sizeof(float))
    ak_accessorMakeFloat(acc);

  if (acc->componentType != AKT_FLOAT
      || acc->normalized
      || acc->bytesPerComponent != sizeof(float)
      || !acc->buffer
      || !acc->buffer->data)
    return false;

  /* Keep B = cross(N, T) * w consistent across reflections.
     Morph tangent deltas are VEC3 and have no handedness component. */
  flipW      = tangent && acc->componentCount == 4
               && (oldCoordSys->rotDirection + 1) * (newCoordSys->rotDirection + 1) < 0;
  rowBytes   = (size_t)acc->componentCount * sizeof(float);
  valueBytes = (flipW ? 4u : 3u) * sizeof(float);
  stride     = acc->byteStride ? acc->byteStride : rowBytes;

  if (stride < valueBytes
      || acc->byteOffset > acc->buffer->length)
    return false;

  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (last > acc->buffer->length
      || valueBytes > acc->buffer->length - last)
    return false;

  if (!ak_coordBufferWritable(acc->buffer))
    return false;

  data = (unsigned char *)acc->buffer->data + acc->byteOffset;
  ak_coordAxisAccessors(oldCoordSys, newCoordSys, &a0, &a1);

  offset[a1.right] = (size_t)a0.right * sizeof(float);
  offset[a1.up]    = (size_t)a0.up    * sizeof(float);
  offset[a1.fwd]   = (size_t)a0.fwd   * sizeof(float);
  sign[a1.right]   = noSign ? 1.0f : (float)(a0.s_right * a1.s_right);
  sign[a1.up]      = noSign ? 1.0f : (float)(a0.s_up    * a1.s_up);
  sign[a1.fwd]     = noSign ? 1.0f : (float)(a0.s_fwd   * a1.s_fwd);

  for (i = 0; i < acc->count; i++) {
    row = data + (size_t)i * stride;
    memcpy(&values[0], row + offset[0], sizeof(float));
    memcpy(&values[1], row + offset[1], sizeof(float));
    memcpy(&values[2], row + offset[2], sizeof(float));

    values[0] *= sign[0];
    values[1] *= sign[1];
    values[2] *= sign[2];
    memcpy(row, values, sizeof(values));

    if (flipW) {
      memcpy(&w, row + sizeof(values), sizeof(w));
      w = -w;
      memcpy(row + sizeof(values), &w, sizeof(w));
    }
  }

  return true;
}

AK_HIDE
bool
ak_coordCvtAccessorVec3(AkAccessor * __restrict acc,
                        AkCoordSys * __restrict oldCoordSys,
                        AkCoordSys * __restrict newCoordSys,
                        bool                    noSign) {
  return ak_coord_cvt_accessor_vector(acc, oldCoordSys, newCoordSys, noSign, false);
}

AK_HIDE
bool
ak_coordCvtAccessorTangent(AkAccessor * __restrict acc,
                           AkCoordSys * __restrict oldCoordSys,
                           AkCoordSys * __restrict newCoordSys) {
  return ak_coord_cvt_accessor_vector(acc, oldCoordSys, newCoordSys, false, true);
}
