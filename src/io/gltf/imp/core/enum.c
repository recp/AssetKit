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

#include "enum.h"
#include "../../../../string_fast.h"

AK_HIDE AkEnum
gltf_enumInputSemantic(const char * name) {
  size_t len;

  if (!name)
    return AK_INPUT_OTHER;

  len = strlen(name);
  switch (len) {
    case _s_gltf_COLOR_len:
      if (ak_str_eq_packed_fast(name,
                                len,
                                _s_gltf_COLOR_u64_exact,
                                _s_gltf_COLOR_len))
        return AK_INPUT_COLOR;
      break;
    case _s_gltf_JOINTS_len:
      if (ak_str_eq_packed_fast(name,
                                len,
                                _s_gltf_JOINTS_u64_exact,
                                _s_gltf_JOINTS_len))
        return AK_INPUT_JOINT;
      if (ak_str_eq_packed_fast(name,
                                len,
                                _s_gltf_NORMAL_u64_exact,
                                _s_gltf_NORMAL_len))
        return AK_INPUT_NORMAL;
      break;
    case _s_gltf_TANGENT_len:
      if (ak_str_eq_packed_fast(name,
                                len,
                                _s_gltf_TANGENT_u64_exact,
                                _s_gltf_TANGENT_len))
        return AK_INPUT_TANGENT;
      if (ak_str_eq_packed_fast(name,
                                len,
                                _s_gltf_WEIGHTS_u64_exact,
                                _s_gltf_WEIGHTS_len))
        return AK_INPUT_WEIGHT;
      break;
    case _s_gltf_POSITION_len:
      if (ak_str_eq_packed_fast(name,
                                len,
                                _s_gltf_POSITION_u64_exact,
                                _s_gltf_POSITION_len))
        return AK_INPUT_POSITION;
      if (ak_str_eq_packed_fast(name,
                                len,
                                _s_gltf_TEXCOORD_u64_exact,
                                _s_gltf_TEXCOORD_len))
        return AK_INPUT_TEXCOORD;
      break;
    default: break;
  }

  if (ak_str_eq_ci_len_fast(name, len, _s_gltf_COLOR, _s_gltf_COLOR_len))
    return AK_INPUT_COLOR;
  if (ak_str_eq_ci_len_fast(name, len, _s_gltf_JOINTS, _s_gltf_JOINTS_len))
    return AK_INPUT_JOINT;
  if (ak_str_eq_ci_len_fast(name, len, _s_gltf_NORMAL, _s_gltf_NORMAL_len))
    return AK_INPUT_NORMAL;
  if (ak_str_eq_ci_len_fast(name, len, _s_gltf_POSITION, _s_gltf_POSITION_len))
    return AK_INPUT_POSITION;
  if (ak_str_eq_ci_len_fast(name, len, _s_gltf_TANGENT, _s_gltf_TANGENT_len))
    return AK_INPUT_TANGENT;
  if (ak_str_eq_ci_len_fast(name, len, _s_gltf_TEXCOORD, _s_gltf_TEXCOORD_len))
    return AK_INPUT_TEXCOORD;
  if (ak_str_eq_ci_len_fast(name, len, _s_gltf_WEIGHTS, _s_gltf_WEIGHTS_len))
    return AK_INPUT_WEIGHT;

  return AK_INPUT_OTHER;
}

AK_HIDE AkEnum
gltf_componentType(int type) {
  switch (type) {
    case 5120:  return AKT_BYTE;   break;
    case 5121:  return AKT_UBYTE;  break;
    case 5122:  return AKT_SHORT;  break;
    case 5123:  return AKT_USHORT; break;
    case 5125:  return AKT_UINT;   break;
    case 5126:  return AKT_FLOAT;  break;
    default: break;
  }
  return AKT_NONE;
}

AK_HIDE int
gltf_componentLen(int type) {
  switch (type) {
    case 5120:            /* AKT_BYTE   */
    case 5121:  return 1; /* AKT_UBYTE  */
    case 5122:            /* AKT_SHORT  */
    case 5123:  return 2; /* AKT_USHORT */
    case 5125:            /* AKT_UINT   */
    case 5126:  return 4; /* AKT_FLOAT  */
    default: return 1;
  }
}

AK_HIDE AkComponentSize
gltf_type(const json_t * __restrict json) {
  const char *value;

  if (!json || !json->value)
    return AK_COMPONENT_SIZE_UNKNOWN;

  value = json->value;
  switch (json->valsize) {
    case _s_gltf_VEC2_len:
      switch (ak_str_pack4_fast(value, _s_gltf_VEC2_len)) {
        case _s_gltf_VEC2_u32_exact: return AK_COMPONENT_SIZE_VEC2;
        case _s_gltf_VEC3_u32_exact: return AK_COMPONENT_SIZE_VEC3;
        case _s_gltf_VEC4_u32_exact: return AK_COMPONENT_SIZE_VEC4;
        case _s_gltf_MAT2_u32_exact: return AK_COMPONENT_SIZE_MAT2;
        case _s_gltf_MAT3_u32_exact: return AK_COMPONENT_SIZE_MAT3;
        case _s_gltf_MAT4_u32_exact: return AK_COMPONENT_SIZE_MAT4;
        default: break;
      }
      break;
    case _s_gltf_SCALAR_len:
      if (ak_str_eq_packed_fast(value,
                                (size_t)json->valsize,
                                _s_gltf_SCALAR_u64_exact,
                                _s_gltf_SCALAR_len))
        return AK_COMPONENT_SIZE_SCALAR;
      break;
    default:
      break;
  }

  return AK_COMPONENT_SIZE_UNKNOWN;
}

AK_HIDE AkEnum
gltf_minFilter(int type) {
  switch (type) {
    case 9728:  return AK_MAGFILTER_NEAREST;       break;
    case 9729:  return AK_MAGFILTER_LINEAR;        break;

    case 9984:  return AK_NEAREST_MIPMAP_NEAREST;  break;
    case 9985:  return AK_LINEAR_MIPMAP_NEAREST;   break;
    case 9986:  return AK_NEAREST_MIPMAP_LINEAR;   break;
    case 9987:  return AK_LINEAR_MIPMAP_LINEAR;    break;
    default: break;
  }
  return 0;
}

AK_HIDE AkEnum
gltf_magFilter(int type) {
  switch (type) {
    case 9728:  return AK_MINFILTER_NEAREST;   break;
    case 9729:  return AK_MINFILTER_LINEAR;    break;
    default: break;
  }
  return 0;
}

AK_HIDE AkEnum
gltf_wrapMode(int type) {
  switch (type) {
    case 33071:  return AK_WRAP_MODE_CLAMP;       break;
    case 33648:  return AK_WRAP_MODE_MIRROR;      break;
    case 10497:  return AK_WRAP_MODE_WRAP;        break;
    default: break;
  }
  return AK_WRAP_MODE_WRAP;
}

AK_HIDE AkInterpolationType
gltf_interp(const json_t * __restrict json) {
  AkEnum val;
  long   glenums_len;
  long   i;

  static const dae_enum glenums[] = {
    {_s_gltf_LINEAR,       AK_INTERPOLATION_LINEAR},
    {_s_gltf_STEP,         AK_INTERPOLATION_STEP},
    {_s_gltf_CUBICSPLINE,  AK_INTERPOLATION_HERMITE}
  };

  val         = AK_INTERPOLATION_LINEAR;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (json_val_eq(json, glenums[i].name)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}
