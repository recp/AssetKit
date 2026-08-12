/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef assetkit_anim_accessor_h
#define assetkit_anim_accessor_h

#include "../common.h"

typedef struct AkAnimAccessorView {
  const unsigned char *data;
  size_t               stride;
  size_t               rowBytes;
  uint32_t             count;
  uint32_t             components;
} AkAnimAccessorView;

AK_INLINE bool
ak_animAccessorView(AkAccessor       * __restrict accessor,
                    AkTypeId                       componentType,
                    size_t                         componentBytes,
                    AkAnimAccessorView * __restrict view) {
  AkBuffer *buffer;
  size_t    rowBytes, stride, lastOffset, required;

  if (!accessor || !view || !componentBytes
      || accessor->componentType != componentType
      || !accessor->componentCount
      || accessor->bytesPerComponent != componentBytes
      || !(buffer = accessor->buffer)
      || !buffer->data)
    return false;
  if ((size_t)accessor->componentCount > SIZE_MAX / componentBytes)
    return false;

  rowBytes = (size_t)accessor->componentCount * componentBytes;
  stride   = accessor->byteStride ? accessor->byteStride : rowBytes;
  if (stride < rowBytes || accessor->byteOffset > buffer->length)
    return false;

  required = 0u;
  if (accessor->count > 0u) {
    if ((size_t)(accessor->count - 1u) > SIZE_MAX / stride)
      return false;
    lastOffset = (size_t)(accessor->count - 1u) * stride;
    if (lastOffset > SIZE_MAX - rowBytes)
      return false;
    required = lastOffset + rowBytes;
    if (required > buffer->length - accessor->byteOffset)
      return false;
    if (accessor->byteLength && required > accessor->byteLength)
      return false;
  }

  view->data       = (const unsigned char *)buffer->data
                     + accessor->byteOffset;
  view->stride     = stride;
  view->rowBytes   = rowBytes;
  view->count      = accessor->count;
  view->components = accessor->componentCount;
  return true;
}

AK_INLINE bool
ak_animAccessorFloatView(AkAccessor       * __restrict accessor,
                         AkAnimAccessorView * __restrict view) {
  return ak_animAccessorView(accessor, AKT_FLOAT, sizeof(float), view);
}

AK_INLINE bool
ak_animAccessorUByteView(AkAccessor       * __restrict accessor,
                         AkAnimAccessorView * __restrict view) {
  return ak_animAccessorView(accessor, AKT_UBYTE, sizeof(uint8_t), view);
}

AK_INLINE bool
ak_animAccessorReadFloat(const AkAnimAccessorView * __restrict view,
                         uint32_t                                row,
                         uint32_t                                component,
                         float                                  *value) {
  const unsigned char *src;

  if (!view || !value || row >= view->count || component >= view->components)
    return false;
  src = view->data + (size_t)row * view->stride
        + (size_t)component * sizeof(float);
  memcpy(value, src, sizeof(*value));
  return true;
}

AK_INLINE bool
ak_animAccessorReadUByte(const AkAnimAccessorView * __restrict view,
                         uint32_t                                row,
                         uint32_t                                component,
                         uint8_t                                *value) {
  if (!view || !value || row >= view->count || component >= view->components)
    return false;
  *value = view->data[(size_t)row * view->stride + component];
  return true;
}

AK_INLINE bool
ak_animAccessorFiniteFloatRange(AkAccessor * __restrict accessor,
                                float      * __restrict outMin,
                                float      * __restrict outMax) {
  AkAnimAccessorView view;
  uint32_t           i;
  float              value, minValue, maxValue;

  if (!outMin || !outMax
      || !ak_animAccessorFloatView(accessor, &view)
      || view.components != 1u || view.count == 0u)
    return false;
  if (!ak_animAccessorReadFloat(&view, 0u, 0u, &minValue)
      || !isfinite(minValue))
    return false;
  maxValue = minValue;
  for (i = 1u; i < view.count; i++) {
    if (!ak_animAccessorReadFloat(&view, i, 0u, &value)
        || !isfinite(value))
      return false;
    if (value < minValue) minValue = value;
    if (value > maxValue) maxValue = value;
  }
  *outMin = minValue;
  *outMax = maxValue;
  return true;
}

#endif /* assetkit_anim_accessor_h */
