/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "decoder.h"
#include "../core/ext.h"

typedef enum AkGLTFMeshoptMode {
  AK_GLTF_MESHOPT_MODE_UNKNOWN = 0,
  AK_GLTF_MESHOPT_MODE_ATTRIBUTES,
  AK_GLTF_MESHOPT_MODE_TRIANGLES,
  AK_GLTF_MESHOPT_MODE_INDICES
} AkGLTFMeshoptMode;

typedef enum AkGLTFMeshoptFilter {
  AK_GLTF_MESHOPT_FILTER_NONE = 0,
  AK_GLTF_MESHOPT_FILTER_OCTAHEDRAL,
  AK_GLTF_MESHOPT_FILTER_QUATERNION,
  AK_GLTF_MESHOPT_FILTER_EXPONENTIAL
} AkGLTFMeshoptFilter;

static
AkGLTFMeshoptMode
gltf_ext_meshoptMode(const json_t * __restrict jmode) {
  if (!jmode)
    return AK_GLTF_MESHOPT_MODE_UNKNOWN;

  if (json_val_eq(jmode, _s_gltf_ATTRIBUTES))
    return AK_GLTF_MESHOPT_MODE_ATTRIBUTES;
  if (json_val_eq(jmode, _s_gltf_TRIANGLES))
    return AK_GLTF_MESHOPT_MODE_TRIANGLES;
  if (json_val_eq(jmode, _s_gltf_INDICES))
    return AK_GLTF_MESHOPT_MODE_INDICES;

  return AK_GLTF_MESHOPT_MODE_UNKNOWN;
}

static
AkGLTFMeshoptFilter
gltf_ext_meshoptFilter(const json_t * __restrict jfilter) {
  if (!jfilter || json_val_eq(jfilter, _s_gltf_NONE))
    return AK_GLTF_MESHOPT_FILTER_NONE;

  if (json_val_eq(jfilter, _s_gltf_OCTAHEDRAL))
    return AK_GLTF_MESHOPT_FILTER_OCTAHEDRAL;
  if (json_val_eq(jfilter, _s_gltf_QUATERNION))
    return AK_GLTF_MESHOPT_FILTER_QUATERNION;
  if (json_val_eq(jfilter, _s_gltf_EXPONENTIAL))
    return AK_GLTF_MESHOPT_FILTER_EXPONENTIAL;

  return AK_GLTF_MESHOPT_FILTER_NONE;
}

AK_HIDE
bool
gltf_ext_bufferView(AkGLTFState  * __restrict gst,
                    AkBufferView * __restrict buffView,
                    const json_t * __restrict jext) {
  const json_t          *jmo;
  const json_t          *it;
  AkBuffer              *srcBuff;
  AkBuffer              *dstBuff;
  const unsigned char   *src;
  size_t                 srcOff;
  size_t                 srcLen;
  size_t                 stride;
  size_t                 count;
  size_t                 dstLen;
  int32_t                buffIdx;
  AkGLTFMeshoptMode      mode;
  AkGLTFMeshoptFilter    filter;

  if (!gst || !buffView || !jext)
    return true;

  jmo = json_get(jext, _s_gltf_EXT_meshopt_compression);
  if (!jmo)
    jmo = json_get(jext, _s_gltf_KHR_meshopt_compression);
  if (!jmo)
    return true;

  if (!gltf_ext_meshopt(gst)) {
    if (buffView->buffer && buffView->buffer->data)
      return true;
    return false;
  }

  it      = json_get(jmo, _s_gltf_buffer);
  buffIdx = it ? json_int32(it, -1) : -1;
  if (buffIdx < 0
      || !(srcBuff = gltf_buffer_at(gst, buffIdx))
      || !srcBuff->data)
    return false;

  srcOff = (it = json_get(jmo, _s_gltf_byteOffset))
             ? (size_t)json_uint64(it, 0) : 0;
  srcLen = (it = json_get(jmo, _s_gltf_byteLength))
             ? (size_t)json_uint64(it, 0) : 0;
  stride = (it = json_get(jmo, _s_gltf_byteStride))
             ? (size_t)json_uint64(it, 0) : 0;
  count  = (it = json_get(jmo, _s_gltf_count))
             ? (size_t)json_uint64(it, 0) : 0;
  mode   = gltf_ext_meshoptMode(json_get(jmo, _s_gltf_mode));
  filter = gltf_ext_meshoptFilter(json_get(jmo, _s_gltf_filter));

  if (srcOff > srcBuff->length
      || srcLen == 0
      || srcLen > srcBuff->length - srcOff
      || stride == 0
      || count == 0)
    return false;

  dstLen = buffView->byteLength;
  if (dstLen == 0) {
    if (count > SIZE_MAX / stride)
      return false;
    dstLen = count * stride;
  }

  dstBuff         = ak_heap_calloc(gst->heap, gst->doc, sizeof(*dstBuff));
  dstBuff->data   = ak_heap_alloc(gst->heap, dstBuff, dstLen);
  dstBuff->length = dstLen;
  src             = (const unsigned char *)srcBuff->data + srcOff;

  if (!gltf_ext_meshoptDecode(gst,
                              dstBuff->data,
                              dstLen,
                              src,
                              srcLen,
                              count,
                              stride,
                              mode,
                              filter))
    return false;

  buffView->buffer     = dstBuff;
  buffView->byteOffset = 0;
  buffView->byteLength = dstLen;
  buffView->byteStride = stride;

  return true;
}
