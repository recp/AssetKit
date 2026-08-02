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
ak_srgb_to_linearf_fast(float channel) {
  float    scaled;
  float    lo;
  uint32_t index;

  /* preserve the reference function outside the encoded display range. */
  if (channel <= 0.0f || channel >= 1.0f)
    return ak_sRGB_linearf(channel);

  scaled = channel * 255.0f;
  index  = (uint32_t)scaled;
  lo     = ak_srgb8_to_linear_table[index];
  return lo + (ak_srgb8_to_linear_table[index + 1u] - lo)
              * (scaled - (float)index);
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

#endif /* ak_color_h */
