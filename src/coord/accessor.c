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
ak_coordCvtAccessorVec3(AkAccessor * __restrict acc,
                        AkCoordSys * __restrict oldCoordSys,
                        AkCoordSys * __restrict newCoordSys,
                        bool                    noSign) {
  AkAxisAccessor a0, a1;
  unsigned char *data;
  size_t         rowBytes, stride, last;
  uint32_t       i;

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

  rowBytes = (size_t)acc->componentCount * sizeof(float);
  stride   = acc->byteStride ? acc->byteStride : rowBytes;
  if (stride < sizeof(float) * 3u
      || acc->byteOffset > acc->buffer->length)
    return false;

  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (last > acc->buffer->length
      || sizeof(float) * 3u > acc->buffer->length - last)
    return false;

  data = (unsigned char *)acc->buffer->data + acc->byteOffset;
  ak_coordAxisAccessors(oldCoordSys, newCoordSys, &a0, &a1);

  for (i = 0; i < acc->count; i++) {
    unsigned char *row;
    float          values[3];
    float  tmp[3];

    row = data + (size_t)i * stride;
    memcpy(values, row, sizeof(values));
    if (noSign) {
      AK_CVT_VEC_NOSIGN(values);
    } else {
      AK_CVT_VEC(values);
    }
    memcpy(row, values, sizeof(values));
  }

  return true;
}
