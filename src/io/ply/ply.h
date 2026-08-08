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

#ifndef ply_h
#define ply_h

#include "common.h"
#include "../../color.h"

#include <math.h>

AK_HIDE
AkResult
ply_ply(AkDoc ** __restrict dest, const char * __restrict filepath);

AK_HIDE
AkResult
ply_export(AkDoc * __restrict doc, const char * __restrict filepath);

AK_HIDE
void
ply_ascii(char * __restrict src, PLYState * __restrict pst);

AK_HIDE
void
ply_bin(char * __restrict src, PLYState * __restrict pst, bool le);

AK_HIDE
void
ply_finish(PLYState * __restrict pst);

AK_HIDE
void
ply_prepare_color_normalization(PLYState * __restrict pst);

AK_INLINE
void
ply_record_alpha_row(PLYState    * __restrict pst,
                     const float * __restrict row,
                     uint32_t                  rowIndex) {
  float alpha;

  if (!pst->alphaBlendBits
      || rowIndex >= pst->ac_rgb->count
      || pst->colorComponentCount < 4u)
    return;

  alpha = row[pst->colorSlot + 3u];
  if (!isfinite(alpha) || alpha < 1.0f)
    pst->alphaBlendBits[rowIndex >> 3u]
      |= (uint8_t)(1u << (rowIndex & 7u));
}

AK_INLINE
void
ply_normalize_color_row(PLYState * __restrict pst,
                        float    * __restrict row,
                        uint32_t               rowIndex) {
  float *color;

  if (!pst->normalizeColors) {
    ply_record_alpha_row(pst, row, rowIndex);
    return;
  }

  color = row + pst->colorSlot;

  if (pst->colorLookup8) {
    color[0] = ak_srgb8_to_linearf_fast((uint8_t)color[0]);
    color[1] = ak_srgb8_to_linearf_fast((uint8_t)color[1]);
    color[2] = ak_srgb8_to_linearf_fast((uint8_t)color[2]);
  } else {
    color[0] *= pst->colorScale;
    color[1] *= pst->colorScale;
    color[2] *= pst->colorScale;
    if (pst->colorSrgb) {
      color[0] = ak_srgb_to_linearf_fast(color[0]);
      color[1] = ak_srgb_to_linearf_fast(color[1]);
      color[2] = ak_srgb_to_linearf_fast(color[2]);
    }
  }

  if (pst->colorComponentCount > 3u)
    color[3] *= pst->colorScale;

  ply_record_alpha_row(pst, row, rowIndex);
}

#endif /* stl_h */
