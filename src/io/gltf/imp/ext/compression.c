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

  if (GLTF_JSON_VAL_EQ(jmode, ATTRIBUTES))
    return AK_GLTF_MESHOPT_MODE_ATTRIBUTES;
  if (GLTF_JSON_VAL_EQ(jmode, TRIANGLES))
    return AK_GLTF_MESHOPT_MODE_TRIANGLES;
  if (GLTF_JSON_VAL_EQ8(jmode, INDICES))
    return AK_GLTF_MESHOPT_MODE_INDICES;

  return AK_GLTF_MESHOPT_MODE_UNKNOWN;
}

static
AkGLTFMeshoptFilter
gltf_ext_meshoptFilter(const json_t * __restrict jfilter) {
  if (!jfilter || GLTF_JSON_VAL_EQ8(jfilter, NONE))
    return AK_GLTF_MESHOPT_FILTER_NONE;

  if (GLTF_JSON_VAL_EQ(jfilter, OCTAHEDRAL))
    return AK_GLTF_MESHOPT_FILTER_OCTAHEDRAL;
  if (GLTF_JSON_VAL_EQ(jfilter, QUATERNION))
    return AK_GLTF_MESHOPT_FILTER_QUATERNION;
  if (GLTF_JSON_VAL_EQ(jfilter, EXPONENTIAL))
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

  jmo = GLTF_JSON_GET(jext, EXT_meshopt_compression);
  if (!jmo)
    jmo = GLTF_JSON_GET(jext, KHR_meshopt_compression);
  if (!jmo)
    return true;

  if (!gltf_ext_meshopt(gst)) {
    if (buffView->buffer && buffView->buffer->data)
      return true;
    return false;
  }

  it      = GLTF_JSON_GET8(jmo, buffer);
  buffIdx = it ? json_int32(it, -1) : -1;
  if (buffIdx < 0
      || !(srcBuff = gltf_buffer_at(gst, buffIdx))
      || !srcBuff->data)
    return false;

  srcOff = (it = GLTF_JSON_GET(jmo, byteOffset))
             ? (size_t)json_uint64(it, 0) : 0;
  srcLen = (it = GLTF_JSON_GET(jmo, byteLength))
             ? (size_t)json_uint64(it, 0) : 0;
  stride = (it = GLTF_JSON_GET(jmo, byteStride))
             ? (size_t)json_uint64(it, 0) : 0;
  count  = (it = GLTF_JSON_GET8(jmo, count))
             ? (size_t)json_uint64(it, 0) : 0;
  mode   = gltf_ext_meshoptMode(GLTF_JSON_GET8(jmo, mode));
  filter = gltf_ext_meshoptFilter(GLTF_JSON_GET8(jmo, filter));

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
