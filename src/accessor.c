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

#include "accessor.h"
#include "default/semantic.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static
float
ak_halfToFloat(uint16_t half) {
  uint32_t sign;
  uint32_t exp;
  uint32_t mant;
  uint32_t bits;
  float    out;

  sign = ((uint32_t)half & 0x8000u) << 16;
  exp  = ((uint32_t)half >> 10) & 0x1fu;
  mant = (uint32_t)half & 0x03ffu;

  if (exp == 0) {
    if (mant == 0) {
      bits = sign;
    } else {
      exp = 127u - 15u + 1u;
      while ((mant & 0x0400u) == 0) {
        mant <<= 1;
        exp--;
      }
      mant &= 0x03ffu;
      bits = sign | (exp << 23) | (mant << 13);
    }
  } else if (exp == 31u) {
    bits = sign | 0x7f800000u | (mant << 13);
  } else {
    bits = sign | ((exp + (127u - 15u)) << 23) | (mant << 13);
  }

  memcpy(&out, &bits, sizeof(out));
  return out;
}

AkAccessor*
ak_accessor_dup(AkAccessor *oldacc) {
  AkHeap      *heap;
  AkAccessor  *acc;

  heap = ak_heap_getheap(oldacc);
  acc  = ak_heap_alloc(heap, ak_mem_parent(oldacc), sizeof(*acc));

  memcpy(acc, oldacc, sizeof(*oldacc));
  ak_setypeid(acc, AKT_ACCESSOR);

  return acc;
}

/* Read one component from a quantized source pointer as float. The
   componentType describes the bytes actually under `src`, and `normalized`
   selects between divide-by-typemax (true) and plain cast (false). Used by
   both ak_accessorAsFloat and ak_accessorMakeFloat. */
AK_INLINE
float
ak_accessor_componentToFloat(const void * __restrict src,
                             AkTypeId                componentType,
                             bool                    normalized) {
  switch (componentType) {
    case AKT_FLOAT:
      return *(const float *)src;
    case AKT_HALF: {
      uint16_t val;
      memcpy(&val, src, sizeof(val));
      return ak_halfToFloat(val);
    }
    case AKT_DOUBLE: {
      double val;
      memcpy(&val, src, sizeof(val));
      return (float)val;
    }
    case AKT_BYTE: {
      float f = (float)(*(const int8_t *)src);
      if (normalized) {
        f /= 127.0f;
        if (f < -1.0f) f = -1.0f;
      }
      return f;
    }
    case AKT_UBYTE: {
      float f = (float)(*(const uint8_t *)src);
      if (normalized) f /= 255.0f;
      return f;
    }
    case AKT_SHORT: {
      float f = (float)(*(const int16_t *)src);
      if (normalized) {
        f /= 32767.0f;
        if (f < -1.0f) f = -1.0f;
      }
      return f;
    }
    case AKT_USHORT: {
      float f = (float)(*(const uint16_t *)src);
      if (normalized) f /= 65535.0f;
      return f;
    }
    case AKT_INT:
      return (float)(*(const int32_t *)src);
    case AKT_UINT:
      return (float)(*(const uint32_t *)src);
    case AKT_INT64: {
      int64_t val;
      memcpy(&val, src, sizeof(val));
      return (float)val;
    }
    case AKT_UINT64: {
      uint64_t val;
      memcpy(&val, src, sizeof(val));
      return (float)val;
    }
    default:
      return 0.0f;
  }
}

AK_EXPORT
size_t
ak_accessorAsFloat(AkAccessor * __restrict acc,
                   float      * __restrict out,
                   size_t                  outCapacity) {
  size_t   needed, perItemBytes;
  uint32_t comps, perComponentBytes, v, c;
  char    *src;
  AkTypeId srcType;
  bool     srcNorm;

  if (!acc || !out
      || !acc->buffer || !acc->buffer->data
      || acc->count == 0)
    return 0;

  comps  = acc->componentCount;
  needed = (size_t)comps * acc->count;
  if (outCapacity < needed) return 0;

  /* Read what's actually in the buffer. When AssetKit dequantized at
     parse time, componentType is AKT_FLOAT and we just copy through;
     when AK_OPT_PRESERVE_QUANTIZED_ATTRS kept the integers, componentType
     still matches the source encoding. originalComponentType is for
     callers who want to know the source format regardless. */
  srcType           = acc->componentType;
  srcNorm           = acc->normalized;
  perComponentBytes = acc->bytesPerComponent;
  perItemBytes      = acc->byteStride
                       ? acc->byteStride
                       : (size_t)comps * perComponentBytes;
  src               = (char *)acc->buffer->data + acc->byteOffset;

  if (srcType == AKT_FLOAT
      && !srcNorm
      && perComponentBytes == sizeof(float)) {
    size_t rowBytes = (size_t)comps * sizeof(float);
    if (perItemBytes < rowBytes
        || acc->byteOffset + (size_t)(acc->count - 1) * perItemBytes + rowBytes
           > acc->buffer->length)
      goto generic_convert;

    if (perItemBytes == rowBytes) {
      memcpy(out, src, rowBytes * acc->count);
      return needed;
    }

    for (v = 0; v < acc->count; v++)
      memcpy(out + (size_t)v * comps, src + (size_t)v * perItemBytes, rowBytes);

    return needed;
  }

generic_convert:
  for (v = 0; v < acc->count; v++) {
    char *vsrc = src + (size_t)v * perItemBytes;
    for (c = 0; c < comps; c++) {
      const void *cp = vsrc + (size_t)c * perComponentBytes;
      out[(size_t)v * comps + c]
        = ak_accessor_componentToFloat(cp, srcType, srcNorm);
    }
  }

  return needed;
}

AK_EXPORT
void
ak_accessorMakeFloat(AkAccessor * __restrict acc) {
  AkHeap   *heap;
  AkBuffer *fbuf;
  size_t    floatBufSize;
  uint32_t  comps;

  if (!acc
      || acc->componentType == AKT_FLOAT
      || !acc->buffer || !acc->buffer->data
      || acc->count == 0
      || acc->fillByteSize == 0)
    return;

  heap         = ak_heap_getheap(acc);
  comps        = acc->componentCount;
  floatBufSize = (size_t)comps * acc->count * sizeof(float);

  /* Parent the new buffer on the accessor itself so its lifetime tracks
     the accessor. The original quantized buffer keeps its existing
     parent linkage, so callers that retained a reference (e.g. an
     external GPU upload using the raw bytes) are unaffected. */
  fbuf         = ak_heap_calloc(heap, acc, sizeof(*fbuf));
  fbuf->data   = ak_heap_alloc(heap, fbuf, floatBufSize);
  fbuf->length = floatBufSize;

  if (!ak_accessorAsFloat(acc, fbuf->data, (size_t)comps * acc->count))
    return;

  acc->buffer            = fbuf;
  acc->byteOffset        = 0;
  acc->bytesPerComponent = sizeof(float);
  acc->fillByteSize      = (size_t)comps * sizeof(float);
  acc->byteStride        = acc->fillByteSize;
  acc->componentType     = AKT_FLOAT;
  acc->normalized        = false;
}
