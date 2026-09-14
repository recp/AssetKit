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
#include "lgsc.h"
#include "../core/ext.h"
#include "../../../../string_fast.h"
#include "../../../../../include/ak/gsplat.h"

static
AkGaussianSplatColorSpace
gltf_gsplatColorSpace(const json_t * __restrict v) {
  const char *s;
  size_t      sz;

  if (!v)
    return AK_GSPLAT_COLOR_SRGB_REC709_DISPLAY;
  if (v->type != JSON_STRING)
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

AK_HIDE
bool
gltf_ext_primitiveGaussianSplat(AkGLTFState     * __restrict gst,
                                AkMeshPrimitive * __restrict prim,
                                const json_t    * __restrict jprim) {
  const json_t    *jext, *jgsplat, *jcomp, *jformat, *jbv, *jlgsc;
  const uint8_t   *bytes;
  json_t         *jkernel, *jcolor, *jproj, *jsort;
  AkGaussianSplat *gs;
  AkBufferView    *bv;
  AkInput         *inp, *previous, *declared, **link;
  AkAccessor      *acc;
  uint32_t         shMask, degree, mask, count, fields, width;
  int32_t          bvIdx;

  if (!gst || !prim || !jprim)
    return true;

  jext    = GLTF_JSON_GET(jprim, extensions);
  jgsplat = jext ? GLTF_JSON_GET(jext, KHR_gaussian_splatting) : NULL;
  if (!jgsplat)
    return true;

  if (jgsplat->type != JSON_OBJECT || prim->type != AK_PRIMITIVE_POINTS
      || !(gs = ak_heap_calloc(gst->heap, prim, sizeof(*gs))))
    return false;

  jkernel = GLTF_JSON_GET8(jgsplat, kernel);
  jcolor  = GLTF_JSON_GET(jgsplat, colorSpace);
  jproj   = GLTF_JSON_GET(jgsplat, projection);
  jsort   = GLTF_JSON_GET(jgsplat, sortingMethod);

  /* Early drafts omitted kernel/colorSpace. Preserve those defaults, but
     never interpret an explicitly different kernel as an ellipse. */
  if ((jkernel && (jkernel->type != JSON_STRING || !GLTF_JSON_VAL_EQ8(jkernel, ellipse)))
      || (jproj && (jproj->type != JSON_STRING || !GLTF_JSON_VAL_EQ(jproj, perspective)))
      || (jsort && (jsort->type != JSON_STRING || !GLTF_JSON_VAL_EQ(jsort, cameraDistance))))
    return false;

  gs->kernel        = AK_GSPLAT_KERNEL_ELLIPSE;
  gs->colorSpace    = gltf_gsplatColorSpace(jcolor);
  gs->projection    = AK_GSPLAT_PROJECTION_PERSPECTIVE;
  gs->sortingMethod = AK_GSPLAT_SORTING_CAMERA_DISTANCE;
  if (gs->colorSpace == AK_GSPLAT_COLOR_UNKNOWN)
    return false;

  prim->gsplat = gs;
  declared     = prim->input;

  jcomp = GLTF_JSON_GET(GLTF_JSON_GET(jgsplat, extensions), KHR_gaussian_splatting_compression_spz_2);
  if (!jcomp)
    jcomp = GLTF_JSON_GET(jgsplat, compression);

  jlgsc = gltf_jsonGetLen(GLTF_JSON_GET(jgsplat, extensions),
                           AK_GLTF_LGSC_EXT, sizeof(AK_GLTF_LGSC_EXT) - 1);
  if (jlgsc && (jcomp || !gltf_lgsc_primitive(gst, prim, jlgsc)))
    return false;

  if (jcomp) {
    if (jcomp->type != JSON_OBJECT)
      return false;

    jformat = GLTF_JSON_GET8(jcomp, format);
    jbv     = GLTF_JSON_GET(jcomp, bufferView);

    if (jformat && (jformat->type != JSON_STRING || !GLTF_JSON_VAL_EQ8(jformat, spz)))
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

    /* Compare before removing placeholders; the decoder owns the final data. */
    for (inp = declared; inp; inp = inp->next)
      if (!inp->accessor || inp->accessor->count != gs->decodedCount)
        return false;
  }

  /* Do not let placeholder accessors shadow the decoded attributes. */
  if (!prim->pos || !prim->pos->accessor || !(count = prim->pos->accessor->count))
    return false;

  shMask = 0;
  fields = 0;
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

    if (previous != inp || !(acc = inp->accessor) || acc->count != count)
      return false;

    width = 3;

    switch (inp->semantic) {
      case AK_INPUT_POSITION: fields |= 1u; break;
      case AK_INPUT_ROTATION: fields |= 2u; width = 4; break;
      case AK_INPUT_SCALE: fields |= 4u; break;
      case AK_INPUT_OPACITY: fields |= 8u; width = 1; break;
      case AK_INPUT_SH:
        if (inp->set >= 25)
          return false;
        break;
      default: width = 0; break;
    }

    if (width && (acc->componentCount != width || acc->componentSize != (AkComponentSize)width))
      return false;

    if (inp->semantic == AK_INPUT_SH && inp->set < 25)
      shMask |= 1u << inp->set;

    link = &inp->next;
  }

  if (fields != 15u || !(shMask & 1u))
    return false;

  for (degree = 0; degree <= 4; degree++) {
    mask = (1u << ((degree + 1) * (degree + 1))) - 1u;
    if (shMask == mask)
      break;
  }

  if (degree > 4)
    return false;

  gs->shDegree = (uint8_t)degree;

  return true;
}
