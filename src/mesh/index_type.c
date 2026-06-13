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
#include "index.h"

typedef struct AkIndexAccessorView {
  AkAccessor accessor;
  AkBuffer   buffer;
} AkIndexAccessorView;

AK_EXPORT
AkTypeId
ak_indexComponentTypeForMax(AkUInt maxIndex) {
  if (maxIndex <= UINT8_MAX)
    return AKT_UBYTE;
  if (maxIndex <= UINT16_MAX)
    return AKT_USHORT;
  return AKT_UINT;
}

AK_EXPORT
size_t
ak_indexComponentSize(AkTypeId componentType) {
  switch (componentType) {
    case AKT_UBYTE:  return sizeof(uint8_t);
    case AKT_USHORT: return sizeof(uint16_t);
    case AKT_UINT:   return sizeof(uint32_t);
    default:         return 0;
  }
}

AK_EXPORT
AkIndexArray*
ak_indexArrayAlloc(AkHeap  * __restrict heap,
                   void    * __restrict parent,
                   size_t               count,
                   AkTypeId             componentType) {
  AkIndexArray *indices;
  size_t        itemSize;

  itemSize = ak_indexComponentSize(componentType);
  if (itemSize == 0)
    return NULL;
  if (count > (SIZE_MAX - sizeof(*indices)) / itemSize)
    return NULL;

  indices = ak_heap_alloc(heap, parent, sizeof(*indices) + itemSize * count);
  if (!indices)
    return NULL;
  indices->count         = count;
  indices->max           = 0;
  indices->componentType = componentType;
  indices->padding       = 0;

  return indices;
}

static
void
ak_indexArrayCopyValues(AkIndexArray * __restrict dst,
                        const AkIndexArray * __restrict src) {
  size_t i, count;

  count = src->count;

  switch (dst->componentType) {
    case AKT_UBYTE: {
      uint8_t *d;

      d = (uint8_t *)dst->items;
      for (i = 0; i < count; i++)
        d[i] = (uint8_t)ak_indexArrayGet(src, i);
      break;
    }
    case AKT_USHORT: {
      uint16_t *d;

      d = (uint16_t *)dst->items;
      for (i = 0; i < count; i++)
        d[i] = (uint16_t)ak_indexArrayGet(src, i);
      break;
    }
    case AKT_UINT: {
      uint32_t *d;

      d = (uint32_t *)dst->items;
      for (i = 0; i < count; i++)
        d[i] = ak_indexArrayGet(src, i);
      break;
    }
    default:
      break;
  }

  dst->max = src->max;
}

AK_EXPORT
AkIndexArray*
ak_indexArrayPromote(AkHeap        * __restrict heap,
                     void          * __restrict parent,
                     AkIndexArray  * __restrict indices,
                     AkTypeId                   componentType) {
  AkIndexArray *promoted;

  if (!indices)
    return NULL;

  if (indices->componentType == componentType)
    return indices;

  promoted = ak_indexArrayAlloc(heap, parent, indices->count, componentType);
  if (!promoted)
    return indices;

  ak_indexArrayCopyValues(promoted, indices);
  ak_free(indices);

  return promoted;
}

AK_EXPORT
AkUInt
ak_indexArrayGet(const AkIndexArray * __restrict indices,
                 size_t                          index) {
  if (!indices || index >= indices->count)
    return 0;

  switch (indices->componentType) {
    case AKT_UBYTE:
      return ((const uint8_t *)indices->items)[index];
    case AKT_USHORT:
      return ((const uint16_t *)indices->items)[index];
    case AKT_UINT:
      return ((const uint32_t *)indices->items)[index];
    default:
      return 0;
  }
}

AK_EXPORT
bool
ak_indexArraySet(AkHeap       * __restrict heap,
                 void         * __restrict parent,
                 AkIndexArray ** __restrict indices,
                 size_t                    index,
                 AkUInt                    value) {
  AkTypeId needed;

  if (!indices || !*indices || index >= (*indices)->count)
    return false;

  needed = ak_indexComponentTypeForMax(value);
  if (ak_indexComponentSize(needed)
      > ak_indexComponentSize((*indices)->componentType))
    *indices = ak_indexArrayPromote(heap, parent, *indices, needed);

  if (ak_indexComponentSize(needed)
      > ak_indexComponentSize((*indices)->componentType))
    return false;

  switch ((*indices)->componentType) {
    case AKT_UBYTE:
      ((uint8_t *)(*indices)->items)[index] = (uint8_t)value;
      break;
    case AKT_USHORT:
      ((uint16_t *)(*indices)->items)[index] = (uint16_t)value;
      break;
    case AKT_UINT:
      ((uint32_t *)(*indices)->items)[index] = value;
      break;
    default:
      return false;
  }

  if (value > (*indices)->max)
    (*indices)->max = value;

  return true;
}

AK_EXPORT
AkUIntArray*
ak_indexArrayMaterializeUInt(AkHeap             * __restrict heap,
                             void               * __restrict parent,
                             const AkIndexArray * __restrict indices) {
  AkUIntArray *out;
  AkUInt      *dst;
  size_t       i, count;

  if (!indices || indices->count == 0)
    return NULL;

  count      = indices->count;
  out        = ak_heap_alloc(heap, parent, sizeof(*out) + sizeof(*dst) * count);
  out->count = count;
  dst        = out->items;

  switch (indices->componentType) {
    case AKT_UBYTE: {
      const uint8_t *src;

      src = (const uint8_t *)indices->items;
      for (i = 0; i < count; i++)
        dst[i] = src[i];
      break;
    }
    case AKT_USHORT: {
      const uint16_t *src;

      src = (const uint16_t *)indices->items;
      for (i = 0; i < count; i++)
        dst[i] = src[i];
      break;
    }
    case AKT_UINT:
      memcpy(dst, indices->items, sizeof(*dst) * count);
      break;
    default:
      ak_free(out);
      return NULL;
  }

  return out;
}

AK_EXPORT
AkUInt
ak_indicesMax(const AkIndexArray * __restrict indices) {
  AkUInt maxIndex;
  size_t i, count;

  if (!indices)
    return 0;

  if (indices->max)
    return indices->max;

  maxIndex = 0;
  count    = indices->count;

  switch (indices->componentType) {
    case AKT_UBYTE: {
      const uint8_t *src;

      src = (const uint8_t *)indices->items;
      for (i = 0; i < count; i++) {
        if (src[i] > maxIndex)
          maxIndex = src[i];
      }
      break;
    }
    case AKT_USHORT: {
      const uint16_t *src;

      src = (const uint16_t *)(const void *)indices->items;
      for (i = 0; i < count; i++) {
        if (src[i] > maxIndex)
          maxIndex = src[i];
      }
      break;
    }
    case AKT_UINT: {
      const uint32_t *src;

      src = (const uint32_t *)(const void *)indices->items;
      for (i = 0; i < count; i++) {
        if (src[i] > maxIndex)
          maxIndex = src[i];
      }
      break;
    }
    default:
      break;
  }

  return maxIndex;
}

AK_EXPORT
AkTypeId
ak_indicesComponentType(const AkIndexArray * __restrict indices) {
  return indices ? indices->componentType : AKT_NONE;
}

static
AkIndexArray*
ak_indexAccessorMaterialize(AkHeap     * __restrict heap,
                            void       * __restrict parent,
                            AkAccessor * __restrict acc,
                            AkTypeId                componentType) {
  AkIndexArray *out;
  char         *data;
  size_t        stride, itemSize, i;
  AkUInt        value, maxIndex;

  if (!acc
      || !acc->buffer
      || !acc->buffer->data
      || acc->count == 0)
    return NULL;

  if (componentType == AKT_NONE)
    componentType = acc->componentType;

  out = ak_indexArrayAlloc(heap, parent, acc->count, componentType);
  if (!out)
    return NULL;

  data     = ((char *)acc->buffer->data) + acc->byteOffset;
  stride   = acc->byteStride ? acc->byteStride : acc->bytesPerComponent;
  itemSize = ak_indexComponentSize(componentType);
  maxIndex = 0;

  if (componentType == acc->componentType && stride == itemSize) {
    memcpy(out->items, data, itemSize * acc->count);
    out->max = ak_indicesMax(out);
    return out;
  }

  switch (acc->componentType) {
    case AKT_UBYTE:
      for (i = 0; i < acc->count; i++) {
        value = *(uint8_t *)(void *)(data + i * stride);
        ak_indexArraySet(heap, parent, &out, i, value);
        if (value > maxIndex)
          maxIndex = value;
      }
      break;
    case AKT_USHORT:
      if (stride == sizeof(uint16_t)
          && (((uintptr_t)data & (sizeof(uint16_t) - 1)) == 0)) {
        uint16_t *src;

        src = (uint16_t *)(void *)data;
        for (i = 0; i < acc->count; i++) {
          value = src[i];
          ak_indexArraySet(heap, parent, &out, i, value);
          if (value > maxIndex)
            maxIndex = value;
        }
      } else {
        for (i = 0; i < acc->count; i++) {
          uint16_t tmp;

          memcpy(&tmp, data + i * stride, sizeof(tmp));
          value = tmp;
          ak_indexArraySet(heap, parent, &out, i, value);
          if (value > maxIndex)
            maxIndex = value;
        }
      }
      break;
    case AKT_UINT:
      if (componentType == AKT_UINT
          && stride == sizeof(uint32_t)
          && (((uintptr_t)data & (sizeof(uint32_t) - 1)) == 0)) {
        uint32_t *dst;

        dst = (uint32_t *)(void *)out->items;
        memcpy(dst, data, sizeof(uint32_t) * acc->count);
        for (i = 0; i < acc->count; i++) {
          if (dst[i] > maxIndex)
            maxIndex = dst[i];
        }
      } else {
        for (i = 0; i < acc->count; i++) {
          uint32_t tmp;

          memcpy(&tmp, data + i * stride, sizeof(tmp));
          value = tmp;
          ak_indexArraySet(heap, parent, &out, i, value);
          if (value > maxIndex)
            maxIndex = value;
        }
      }
      break;
    default:
      ak_free(out);
      return NULL;
  }

  out->max = maxIndex;

  return out;
}

AK_EXPORT
AkIndexArray*
ak_meshPrimitiveMaterializeIndices(AkMeshPrimitive * __restrict prim) {
  AkHeap *heap;

  if (!prim)
    return NULL;

  if (prim->indices)
    return prim->indices;

  heap = ak_heap_getheap(prim);

  if (prim->indexAccessor)
    prim->indices = ak_indexAccessorMaterialize(heap,
                                                prim,
                                                prim->indexAccessor,
                                                AKT_NONE);

  if (prim->indices) {
    prim->indexAccessor = NULL;
  }

  return prim->indices;
}

AK_EXPORT
AkIndexArray*
ak_meshPrimitiveEnsureUIntIndices(AkMeshPrimitive * __restrict prim) {
  AkHeap       *heap;
  AkIndexArray *indices;

  if (!prim)
    return NULL;

  indices = ak_meshPrimitiveMaterializeIndices(prim);
  if (!indices)
    return NULL;

  if (indices->componentType != AKT_UINT) {
    heap          = ak_heap_getheap(prim);
    prim->indices = ak_indexArrayPromote(heap, prim, indices, AKT_UINT);
    indices       = prim->indices;
    prim->indexAccessor = NULL;
  }

  return indices;
}

AK_EXPORT
bool
ak_meshPrimitiveIsTupleIndexed(const AkMeshPrimitive * __restrict prim) {
  return prim && prim->indexStride > 1;
}

AK_EXPORT
bool
ak_meshPrimitiveIsSingleIndexed(const AkMeshPrimitive * __restrict prim) {
  return prim && (prim->indexStride <= 1);
}

AK_EXPORT
AkResult
ak_meshPrimitiveEnsureSingleIndex(AkMeshPrimitive * __restrict prim) {
  AkResult ret;

  if (!prim)
    return AK_EINVAL;

  if (ak_meshPrimitiveIsSingleIndexed(prim))
    return AK_OK;

  if (!prim->mesh)
    return AK_ERR;

  ak_meshBeginEdit(prim->mesh);
  ret = ak_primFixIndicesRetainDuplicator(prim->mesh, prim, true);
  ak_meshEndEdit(prim->mesh);

  return ret;
}

AK_EXPORT
AkAccessor*
ak_meshPrimitiveSingleIndexAccessor(AkMeshPrimitive * __restrict prim) {
  if (ak_meshPrimitiveEnsureSingleIndex(prim) != AK_OK)
    return NULL;

  return ak_meshPrimitiveIndexAccessor(prim);
}

AK_EXPORT
AkAccessor*
ak_meshPrimitiveIndexAccessor(AkMeshPrimitive * __restrict prim) {
  AkHeap              *heap;
  AkIndexAccessorView *view;
  AkBuffer            *buff;
  AkAccessor          *acc;
  AkIndexArray        *indices;
  size_t               itemSize;

  if (!prim)
    return NULL;

  indices = prim->indices;
  acc     = prim->indexAccessor;

  if (!indices)
    return acc;

  itemSize = ak_indexComponentSize(indices->componentType);
  if (itemSize == 0)
    return NULL;
  if (indices->count > UINT32_MAX)
    return NULL;

  if (acc
      && acc->buffer
      && acc->buffer->data == indices->items
      && acc->count == indices->count
      && acc->componentType == indices->componentType)
    return acc;

  heap = ak_heap_getheap(prim);
  view = ak_heap_alloc(heap, prim, sizeof(*view));
  if (!view)
    return NULL;

  acc  = &view->accessor;
  buff = &view->buffer;

  buff->name   = NULL;
  buff->data   = indices->items;
  buff->length = itemSize * indices->count;

  acc->buffer                  = buff;
  acc->name                    = NULL;
  acc->min                     = NULL;
  acc->max                     = NULL;
  acc->byteOffset              = 0;
  acc->byteStride              = itemSize;
  acc->byteLength              = buff->length;
  acc->count                   = (uint32_t)indices->count;
  acc->bytesPerComponent       = (uint32_t)itemSize;
  acc->componentSize           = AK_COMPONENT_SIZE_SCALAR;
  acc->componentType           = indices->componentType;
  acc->componentCount          = 1;
  acc->fillByteSize            = itemSize;
  acc->gpuTarget               = 0;
  acc->normalized              = false;
  acc->originalComponentType   = indices->componentType;
  acc->originallyNormalized    = false;

  prim->indexAccessor = acc;

  return acc;
}

AK_EXPORT
size_t
ak_meshPrimitiveIndexCount(const AkMeshPrimitive * __restrict prim) {
  if (!prim)
    return 0;
  if (prim->indices)
    return prim->indices->count;
  if (prim->indexAccessor)
    return prim->indexAccessor->count;
  return 0;
}

static
AkUInt
ak_indexAccessorMax(const AkAccessor * __restrict acc) {
  const char *data;
  size_t      stride, i;
  AkUInt      maxIndex, value;

  if (!acc
      || !acc->buffer
      || !acc->buffer->data
      || acc->count == 0)
    return 0;

  data   = ((const char *)acc->buffer->data) + acc->byteOffset;
  stride = acc->byteStride ? acc->byteStride : acc->bytesPerComponent;
  maxIndex = 0;

  switch (acc->componentType) {
    case AKT_UBYTE:
      for (i = 0; i < acc->count; i++) {
        value = *(const uint8_t *)(data + i * stride);
        if (value > maxIndex)
          maxIndex = value;
      }
      break;
    case AKT_USHORT:
      if (stride == sizeof(uint16_t)
          && (((uintptr_t)data & (sizeof(uint16_t) - 1)) == 0)) {
        const uint16_t *src;

        src = (const uint16_t *)(const void *)data;
        for (i = 0; i < acc->count; i++) {
          value = src[i];
          if (value > maxIndex)
            maxIndex = value;
        }
      } else {
        for (i = 0; i < acc->count; i++) {
          uint16_t tmp;

          memcpy(&tmp, data + i * stride, sizeof(tmp));
          value = tmp;
          if (value > maxIndex)
            maxIndex = value;
        }
      }
      break;
    case AKT_UINT:
      if (stride == sizeof(uint32_t)
          && (((uintptr_t)data & (sizeof(uint32_t) - 1)) == 0)) {
        const uint32_t *src;

        src = (const uint32_t *)(const void *)data;
        for (i = 0; i < acc->count; i++) {
          value = src[i];
          if (value > maxIndex)
            maxIndex = value;
        }
      } else {
        for (i = 0; i < acc->count; i++) {
          uint32_t tmp;

          memcpy(&tmp, data + i * stride, sizeof(tmp));
          value = tmp;
          if (value > maxIndex)
            maxIndex = value;
        }
      }
      break;
    default:
      break;
  }

  return maxIndex;
}

AK_EXPORT
AkUInt
ak_meshPrimitiveIndexMax(const AkMeshPrimitive * __restrict prim) {
  const AkAccessor *acc;

  if (!prim)
    return 0;
  if (prim->indices) {
    return ak_indicesMax(prim->indices);
  }

  acc = prim->indexAccessor;
  if (acc && acc->count > 0)
    return ak_indexAccessorMax(acc);

  if (prim->pos && prim->pos->accessor && prim->pos->accessor->count > 0)
    return prim->pos->accessor->count - 1;

  return 0;
}

AK_EXPORT
AkTypeId
ak_meshPrimitiveIndexComponentType(const AkMeshPrimitive * __restrict prim) {
  AkTypeId componentType;

  if (!prim)
    return AKT_NONE;

  if (prim->indices)
    return prim->indices->componentType;

  if (prim->indexAccessor) {
    componentType = prim->indexAccessor->componentType;
    if (componentType == AKT_UBYTE
        || componentType == AKT_USHORT
        || componentType == AKT_UINT)
      return componentType;
  }

  if (prim->pos && prim->pos->accessor && prim->pos->accessor->count > 0)
    return ak_indexComponentTypeForMax(prim->pos->accessor->count - 1);

  return AKT_NONE;
}
