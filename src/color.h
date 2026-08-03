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

#ifndef ak_color_h
#define ak_color_h

#include "common.h"

#if defined(__aarch64__) && defined(__ARM_NEON)
#  include <arm_neon.h>
#  define AK_COLOR_HAS_NEON 1
#else
#  define AK_COLOR_HAS_NEON 0
#endif

/* Shared, cache-resident transfer tables. Importers use the first table for
   exact 8-bit decode and interpolation of bulk float colors. Exporters use
   the second table as exact quantization boundaries, avoiding powf for every
   vertex color component. */
AK_HIDE extern const float   ak_srgb8_to_linear_table[256];
AK_HIDE extern const uint8_t ak_srgb8_to_linear8_table[256];
AK_HIDE extern const float   ak_linear_to_srgb8_boundary[255];
AK_HIDE extern const uint8_t ak_linear_to_srgb8_bin[4096];

AK_INLINE
float
ak_srgb8_to_linearf_fast(uint8_t channel) {
  return ak_srgb8_to_linear_table[channel];
}

AK_INLINE
uint8_t
ak_srgb8_to_linear8_fast(uint8_t channel) {
  return ak_srgb8_to_linear8_table[channel];
}

AK_INLINE
float
ak_srgb_to_linearf_unchecked(float channel) {
  float    scaled;
  float    lo;
  uint32_t index;

  scaled = channel * 255.0f;
  index  = (uint32_t)scaled;
  lo     = ak_srgb8_to_linear_table[index];
  return lo + (ak_srgb8_to_linear_table[index + 1u] - lo)
              * (scaled - (float)index);
}

AK_INLINE
float
ak_srgb_to_linearf_fast(float channel) {
  /* Preserve the reference function outside the encoded display range. */
  if (!(channel > 0.0f) || channel >= 1.0f)
    return ak_sRGB_linearf(channel);

  return ak_srgb_to_linearf_unchecked(channel);
}

AK_INLINE
void
ak_srgb3_to_linearf_fast(const float * __restrict src,
                         float       * __restrict dst) {
  float r;
  float g;
  float b;

  r = src[0];
  g = src[1];
  b = src[2];

  /* Vertex colors overwhelmingly stay in the display range. Validate the
     triplet once so the common path avoids three independent range branches. */
  if (r > 0.0f && r < 1.0f
      && g > 0.0f && g < 1.0f
      && b > 0.0f && b < 1.0f) {
#if AK_COLOR_HAS_NEON
    float32x4_t encoded;
    float32x4_t fraction;
    float32x4_t high;
    float32x4_t linear;
    float32x4_t low;
    float32x4_t scaled;
    uint32x4_t index;
    uint32_t i0;
    uint32_t i1;
    uint32_t i2;

    encoded = vdupq_n_f32(0.0f);
    encoded = vsetq_lane_f32(r, encoded, 0);
    encoded = vsetq_lane_f32(g, encoded, 1);
    encoded = vsetq_lane_f32(b, encoded, 2);
    scaled  = vmulq_n_f32(encoded, 255.0f);
    index   = vcvtq_u32_f32(scaled);
    i0      = vgetq_lane_u32(index, 0);
    i1      = vgetq_lane_u32(index, 1);
    i2      = vgetq_lane_u32(index, 2);

    low  = vdupq_n_f32(0.0f);
    high = vdupq_n_f32(0.0f);
    low  = vsetq_lane_f32(ak_srgb8_to_linear_table[i0], low, 0);
    low  = vsetq_lane_f32(ak_srgb8_to_linear_table[i1], low, 1);
    low  = vsetq_lane_f32(ak_srgb8_to_linear_table[i2], low, 2);
    high = vsetq_lane_f32(ak_srgb8_to_linear_table[i0 + 1u], high, 0);
    high = vsetq_lane_f32(ak_srgb8_to_linear_table[i1 + 1u], high, 1);
    high = vsetq_lane_f32(ak_srgb8_to_linear_table[i2 + 1u], high, 2);

    fraction = vsubq_f32(scaled, vcvtq_f32_u32(index));
    linear   = vfmaq_f32(low, vsubq_f32(high, low), fraction);
    vst1q_f32(dst, linear);
#else
    dst[0] = ak_srgb_to_linearf_unchecked(r);
    dst[1] = ak_srgb_to_linearf_unchecked(g);
    dst[2] = ak_srgb_to_linearf_unchecked(b);
#endif
    return;
  }

  dst[0] = ak_srgb_to_linearf_fast(r);
  dst[1] = ak_srgb_to_linearf_fast(g);
  dst[2] = ak_srgb_to_linearf_fast(b);
}

/* Chain-rule scale for converting a tangent whose value axis is sRGB
   encoded. This is intentionally separate from ak_srgb_to_linearf_fast():
   control-point values use the transfer function itself, while Hermite and
   cardinal tangents use its derivative. */
AK_INLINE
float
ak_srgb_to_linear_derivativef_fast(float channel) {
  float base;

  if (channel <= 0.04045f)
    return 1.0f / 12.92f;

  base = (channel + 0.055f) * (1.0f / 1.055f);
  return (2.4f / 1.055f) * (ak_srgb_to_linearf_fast(channel) / base);
}

AK_INLINE
uint8_t
ak_linear_to_srgb8_fast(float channel) {
  uint32_t bin;
  uint32_t code;

  /* the negated comparison maps NaN to zero as well. */
  if (!(channel > 0.0f))
    return 0u;
  if (channel >= 1.0f)
    return 255u;

  /* A 12-bit direct bin is narrower than the smallest adjacent sRGB byte
     boundary interval. The bin supplies the lower code; one comparison
     resolves the only possible boundary inside it. */
  bin  = (uint32_t)(channel * 4096.0f);
  code = ak_linear_to_srgb8_bin[bin];
  code += (uint32_t)(channel >= ak_linear_to_srgb8_boundary[code]);

  return (uint8_t)code;
}

AK_INLINE
float
ak_linear_to_srgbf_fast(float channel) {
  float    hi;
  float    lo;
  uint32_t code;

  /* Preserve extended-range linear values. Map NaN to a safe serialized
     value, matching the integer encoder above. */
  if (!(channel > 0.0f))
    return channel <= 0.0f ? channel * 12.92f : 0.0f;
  if (channel >= 1.0f)
    return ak_linear_sRGBf(channel);

  /* Start at the nearest 8-bit sRGB code, then select the enclosing table
     interval. Linear interpolation is the exact inverse of the importer’s
     cache-resident piecewise-linear decode. */
  code = ak_linear_to_srgb8_fast(channel);
  code -= (uint32_t)(ak_srgb8_to_linear_table[code] > channel);
  lo    = ak_srgb8_to_linear_table[code];
  hi    = ak_srgb8_to_linear_table[code + 1u];

  return ((float)code + (channel - lo) / (hi - lo)) * (1.0f / 255.0f);
}

#undef AK_COLOR_HAS_NEON

#endif /* ak_color_h */
