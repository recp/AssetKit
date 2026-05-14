/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "decoder.h"
#include "../core/ext.h"
#include "../../../../string_fast.h"
#include "../../../../../include/ak/gsplat.h"

static
AkGaussianSplatColorSpace
gltf_gsplatColorSpace(const json_t * __restrict v) {
  size_t      sz;
  const char *s;

  if (!v)
    return AK_GSPLAT_COLOR_UNKNOWN;

  s  = json_string(v);
  sz = v->valsize;
  if (ak_str_eq_fast(s,
                     sz,
                     _s_gltf_srgb_rec709_display,
                     _s_gltf_srgb_rec709_display_len))
    return AK_GSPLAT_COLOR_SRGB_REC709_DISPLAY;
  if (ak_str_eq_fast(s,
                     sz,
                     _s_gltf_lin_rec709_display,
                     _s_gltf_lin_rec709_display_len))
    return AK_GSPLAT_COLOR_LIN_REC709_DISPLAY;

  return AK_GSPLAT_COLOR_UNKNOWN;
}

static
AkGaussianSplatProjection
gltf_gsplatProjection(const json_t * __restrict v) {
  size_t      sz;
  const char *s;

  if (!v)
    return AK_GSPLAT_PROJECTION_PERSPECTIVE;

  s  = json_string(v);
  sz = v->valsize;
  if (ak_str_eq_fast(s, sz, _s_gltf_orthographic, _s_gltf_orthographic_len))
    return AK_GSPLAT_PROJECTION_ORTHOGRAPHIC;

  return AK_GSPLAT_PROJECTION_PERSPECTIVE;
}

static
AkGaussianSplatSortingMethod
gltf_gsplatSorting(const json_t * __restrict v) {
  size_t      sz;
  const char *s;

  if (!v)
    return AK_GSPLAT_SORTING_CAMERA_DISTANCE;

  s  = json_string(v);
  sz = v->valsize;
  if (ak_str_eq_packed_fast(s, sz, _s_gltf_none_u64_exact, _s_gltf_none_len))
    return AK_GSPLAT_SORTING_NONE;

  return AK_GSPLAT_SORTING_CAMERA_DISTANCE;
}

AK_HIDE
bool
gltf_ext_primitiveGaussianSplat(AkGLTFState     * __restrict gst,
                                AkMeshPrimitive * __restrict prim,
                                const json_t    * __restrict jprim) {
  const json_t    *jext;
  const json_t    *jgsplat;
  json_t          *jkernel;
  json_t          *jcolor;
  json_t          *jproj;
  json_t          *jsort;
  AkGaussianSplat *gs;

  if (!gst || !prim || !jprim)
    return true;

  jext    = GLTF_JSON_GET(jprim, extensions);
  jgsplat = jext ? GLTF_JSON_GET(jext, KHR_gaussian_splatting) : NULL;
  if (!jgsplat)
    return true;

  gs = ak_heap_calloc(gst->heap, prim, sizeof(*gs));

  jkernel = GLTF_JSON_GET8(jgsplat, kernel);
  jcolor  = GLTF_JSON_GET(jgsplat, colorSpace);
  jproj   = GLTF_JSON_GET(jgsplat, projection);
  jsort   = GLTF_JSON_GET(jgsplat, sortingMethod);

  (void)jkernel;
  gs->kernel        = AK_GSPLAT_KERNEL_ELLIPSE;
  gs->colorSpace    = gltf_gsplatColorSpace(jcolor);
  gs->projection    = gltf_gsplatProjection(jproj);
  gs->sortingMethod = gltf_gsplatSorting(jsort);

  prim->gsplat = gs;

  {
    const json_t *jcomp;
    const json_t *jformat;
    const json_t *jbv;
    int32_t       bvIdx;
    AkBufferView *bv;

    if ((jcomp = GLTF_JSON_GET(jgsplat, compression))) {
      jformat = GLTF_JSON_GET8(jcomp, format);
      jbv     = GLTF_JSON_GET(jcomp, bufferView);

      if (jformat && !GLTF_JSON_VAL_EQ8(jformat, spz))
        return false;
      if (!jbv)
        return false;

      bvIdx = json_int32(jbv, -1);
      bv    = gltf_bufferView_at(gst, bvIdx);
      if (!bv || !bv->buffer || !bv->buffer->data || bv->byteLength == 0)
        return false;

      {
        const uint8_t *bytes;

        bytes = (const uint8_t *)bv->buffer->data + bv->byteOffset;
        if (!gltf_ext_spzDecodeBytes(gst, prim, bytes, bv->byteLength))
          return false;
      }
    }
  }

  return true;
}
