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
  const json_t    *jext, *jgsplat, *jcomp, *jformat, *jbv;
  const uint8_t   *bytes;
  json_t         *jkernel, *jcolor, *jproj, *jsort;
  AkGaussianSplat *gs;
  AkBufferView    *bv;
  AkInput         *inp, *previous, **link;
  uint32_t         shMask, degree, mask;
  int32_t          bvIdx;

  if (!gst || !prim || !jprim)
    return true;

  jext    = GLTF_JSON_GET(jprim, extensions);
  jgsplat = jext ? GLTF_JSON_GET(jext, KHR_gaussian_splatting) : NULL;
  if (!jgsplat)
    return true;

  if (prim->type != AK_PRIMITIVE_POINTS
      || !(gs = ak_heap_calloc(gst->heap, prim, sizeof(*gs))))
    return false;

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

  jcomp = gltf_jsonGetLen(GLTF_JSON_GET(jgsplat, extensions),
                          "KHR_gaussian_splatting_compression_spz_2",
                          sizeof("KHR_gaussian_splatting_compression_spz_2") - 1);
  if (!jcomp)
    jcomp = GLTF_JSON_GET(jgsplat, compression);

  if (jcomp) {
    jformat = GLTF_JSON_GET8(jcomp, format);
    jbv     = GLTF_JSON_GET(jcomp, bufferView);

    if (jformat && !GLTF_JSON_VAL_EQ8(jformat, spz))
      return false;

    if (!jbv)
      return false;

    bvIdx = json_int32(jbv, -1);
    bv    = gltf_bufferView_at(gst, bvIdx);
    if (!bv || !bv->buffer || !bv->buffer->data || bv->byteLength == 0
        || bv->byteOffset > bv->buffer->length
        || bv->byteLength > bv->buffer->length - bv->byteOffset)
      return false;

    bytes = (const uint8_t *)bv->buffer->data + bv->byteOffset;
    if (!gltf_ext_spzDecodeBytes(gst, prim, bytes, bv->byteLength))
      return false;
  }

  /* Do not let placeholder accessors shadow the decoded attributes. */
  shMask = 0;
  link   = &prim->input;

  while ((inp = *link)) {
    for (previous = prim->input; previous != inp; previous = previous->next)
      if (previous->semantic == inp->semantic && previous->set == inp->set
          && inp->semantic != AK_INPUT_OTHER)
        break;

    if (previous != inp && gs->decodedCount) {
      *link = inp->next;
      prim->inputCount--;
      ak_free(inp);
      continue;
    }

    if (inp->semantic == AK_INPUT_SH && inp->set < 25)
      shMask |= 1u << inp->set;

    link = &inp->next;
  }

  for (degree = 0; degree <= 4; degree++) {
    mask = (1u << ((degree + 1) * (degree + 1))) - 1u;
    if ((shMask & mask) != mask)
      break;

    gs->shDegree = (uint8_t)degree;
  }

  return true;
}
