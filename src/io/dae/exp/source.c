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

#include "source.h"
#include "../strpool.h"

AK_HIDE
const char*
dae_semantic_name(AkInput * __restrict input) {
  if (!input)
    return NULL;

  switch (input->semantic) {
    case AK_INPUT_POSITION: return _s_dae_POSITION;
    case AK_INPUT_NORMAL:   return _s_dae_NORMAL;
    case AK_INPUT_TANGENT:  return _s_dae_TEXTANGENT;
    case AK_INPUT_TEXCOORD:
    case AK_INPUT_UV:       return _s_dae_TEXCOORD;
    case AK_INPUT_COLOR:    return _s_dae_COLOR;
    case AK_INPUT_JOINT:    return _s_dae_JOINT;
    case AK_INPUT_WEIGHT:   return _s_dae_WEIGHT;
    default:                return input->semanticRaw;
  }
}

AK_HIDE
DAEExpName
dae_param_exp_name(uint32_t idx) {
  switch (idx) {
    case 0:  return DAE_EXP_NAME(X);
    case 1:  return DAE_EXP_NAME(Y);
    case 2:  return DAE_EXP_NAME(Z);
    case 3:  return DAE_EXP_NAME_LIT("W");
    default: return DAE_EXP_NAME_LIT("C");
  }
}

AK_HIDE
DAEExpName
dae_input_param_exp_name(AkInput * __restrict input, uint32_t idx) {
  if (input
      && (input->semantic == AK_INPUT_TEXCOORD
          || input->semantic == AK_INPUT_UV)) {
    switch (idx) {
      case 0:  return DAE_EXP_NAME_LIT("S");
      case 1:  return DAE_EXP_NAME_LIT("T");
      case 2:  return DAE_EXP_NAME_LIT("P");
      default: return DAE_EXP_NAME_LIT("Q");
    }
  }

  return dae_param_exp_name(idx);
}

AK_HIDE
bool
dae_write_source(DAEExpState  * __restrict st,
                 AkInput      * __restrict input,
                 uint32_t                  geomIdx,
                 uint32_t                  primIdx,
                 uint32_t                  inputIdx) {
  DAEExpWriter *w;
  AkAccessor   *acc;
  const char   *semantic;
  float        *scratch;
  DAEExpName    semanticName;
  uint32_t      i;
  uint32_t      c;
  uint32_t      componentCount;
  bool          direct;
  bool          flipTexcoordV;

  w   = &st->w;
  acc = input ? input->accessor : NULL;
  if (!acc || acc->count == 0 || acc->componentCount == 0)
    return false;

  semantic = dae_semantic_name(input);
  if (!semantic || !*semantic)
    return false;

  semanticName   = DAE_EXP_NAME_CSTR(semantic);
  componentCount = acc->componentCount;
  direct         = io_accessor_float_direct(acc);
  flipTexcoordV  = (input->semantic == AK_INPUT_TEXCOORD
                    || input->semantic == AK_INPUT_UV)
                   && (!st->doc->inf || !st->doc->inf->flipImage)
                   && componentCount > 1u;
  scratch        = NULL;

  if (!direct) {
    size_t floatCount;

    if ((size_t)acc->count > (size_t)-1 / componentCount)
      return false;

    floatCount = (size_t)acc->count * componentCount;
    scratch    = dae_scratch(st, sizeof(float) * floatCount);

    if (!scratch || ak_accessorAsFloat(acc, scratch, floatCount) != floatCount)
      return false;
  }

  dae_w_lit(w, "<source id=\"");
  dae_w_geom_prim_id(w, geomIdx, primIdx, semanticName);
  dae_w_ch(w, '_');
  dae_w_uint_fast(w, inputIdx);
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_geom_prim_id(w, geomIdx, primIdx, semanticName);
  dae_w_ch(w, '_');
  dae_w_uint_fast(w, inputIdx);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, (size_t)acc->count * componentCount);
  dae_w_lit(w, "\">");

  for (i = 0; i < acc->count; i++) {
    const float *row;

    row = direct
          ? io_accessor_float_row(acc, i)
          : scratch + (size_t)i * componentCount;

    for (c = 0; c < componentCount; c++) {
      if (i > 0 || c > 0)
        dae_w_ch(w, ' ');
      dae_w_float_fast(w, flipTexcoordV && c == 1u ? 1.0f - row[c] : row[c]);
    }
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_geom_prim_id(w, geomIdx, primIdx, semanticName);
  dae_w_ch(w, '_');
  dae_w_uint_fast(w, inputIdx);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, acc->count);
  dae_w_lit(w, "\" stride=\"");
  dae_w_uint_fast(w, componentCount);
  dae_w_lit(w, "\">");

  for (c = 0; c < componentCount; c++) {
    dae_w_lit(w, "<param name=\"");
    dae_w_name(w, dae_input_param_exp_name(input, c));
    dae_w_lit(w, "\" type=\"float\"/>");
  }

  dae_w_lit(w, "</accessor></technique_common></source>");

  return w->result == AK_OK;
}
