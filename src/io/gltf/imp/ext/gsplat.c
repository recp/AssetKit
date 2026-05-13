/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "decoder.h"
#include "../core/ext.h"
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
  if (sz == 19 && memcmp(s, "srgb_rec709_display", 19) == 0)
    return AK_GSPLAT_COLOR_SRGB_REC709_DISPLAY;
  if (sz == 18 && memcmp(s, "lin_rec709_display", 18) == 0)
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
  if (sz == 12 && memcmp(s, "orthographic", 12) == 0)
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
  if (sz == 4 && memcmp(s, "none", 4) == 0)
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

  jext    = json_get(jprim, _s_gltf_extensions);
  jgsplat = jext ? json_get(jext, _s_gltf_KHR_gaussian_splatting) : NULL;
  if (!jgsplat)
    return true;

  gs = ak_heap_calloc(gst->heap, prim, sizeof(*gs));

  jkernel = json_get(jgsplat, _s_gltf_kernel);
  jcolor  = json_get(jgsplat, _s_gltf_colorSpace);
  jproj   = json_get(jgsplat, _s_gltf_projection);
  jsort   = json_get(jgsplat, _s_gltf_sortingMethod);

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

    if ((jcomp = json_get(jgsplat, _s_gltf_compression))) {
      jformat = json_get(jcomp, _s_gltf_format);
      jbv     = json_get(jcomp, _s_gltf_bufferView);

      if (jformat && !json_val_eq(jformat, _s_gltf_spz))
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
