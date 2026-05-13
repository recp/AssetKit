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

AK_HIDE AkEnum
gltf_enumInputSemantic(const char * name) {
  if (!name)
    return AK_INPUT_OTHER;

  switch (name[0]) {
    case 'C':
      if (strcmp(name, _s_gltf_COLOR) == 0) return AK_INPUT_COLOR;
      break;
    case 'J':
      if (strcmp(name, _s_gltf_JOINTS) == 0) return AK_INPUT_JOINT;
      break;
    case 'N':
      if (strcmp(name, _s_gltf_NORMAL) == 0) return AK_INPUT_NORMAL;
      break;
    case 'P':
      if (strcmp(name, _s_gltf_POSITION) == 0) return AK_INPUT_POSITION;
      break;
    case 'T':
      if (strcmp(name, _s_gltf_TANGENT) == 0) return AK_INPUT_TANGENT;
      if (strcmp(name, _s_gltf_TEXCOORD) == 0) return AK_INPUT_TEXCOORD;
      break;
    case 'W':
      if (strcmp(name, _s_gltf_WEIGHTS) == 0) return AK_INPUT_WEIGHT;
      break;
    default:
      break;
  }

  if (strcasecmp(name, _s_gltf_COLOR) == 0)    return AK_INPUT_COLOR;
  if (strcasecmp(name, _s_gltf_JOINTS) == 0)   return AK_INPUT_JOINT;
  if (strcasecmp(name, _s_gltf_NORMAL) == 0)   return AK_INPUT_NORMAL;
  if (strcasecmp(name, _s_gltf_POSITION) == 0) return AK_INPUT_POSITION;
  if (strcasecmp(name, _s_gltf_TANGENT) == 0)  return AK_INPUT_TANGENT;
  if (strcasecmp(name, _s_gltf_TEXCOORD) == 0) return AK_INPUT_TEXCOORD;
  if (strcasecmp(name, _s_gltf_WEIGHTS) == 0)  return AK_INPUT_WEIGHT;

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
    case 4:
      if (memcmp(value, _s_gltf_VEC2, 4) == 0)
        return AK_COMPONENT_SIZE_VEC2;
      if (memcmp(value, _s_gltf_VEC3, 4) == 0)
        return AK_COMPONENT_SIZE_VEC3;
      if (memcmp(value, _s_gltf_VEC4, 4) == 0)
        return AK_COMPONENT_SIZE_VEC4;
      if (memcmp(value, _s_gltf_MAT2, 4) == 0)
        return AK_COMPONENT_SIZE_MAT2;
      if (memcmp(value, _s_gltf_MAT3, 4) == 0)
        return AK_COMPONENT_SIZE_MAT3;
      if (memcmp(value, _s_gltf_MAT4, 4) == 0)
        return AK_COMPONENT_SIZE_MAT4;
      break;
    case 6:
      if (memcmp(value, _s_gltf_SCALAR, 6) == 0)
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

AK_HIDE AkOpaque
gltf_alphaMode(const json_t * __restrict json) {
  const char *value;

  if (!json || !json->value)
    return AK_OPAQUE_OPAQUE;

  value = json->value;
  switch (json->valsize) {
    case 4:
      if (memcmp(value, _s_gltf_MASK, 4) == 0)
        return AK_OPAQUE_MASK;
      break;
    case 5:
      if (memcmp(value, _s_gltf_BLEND, 5) == 0)
        return AK_OPAQUE_BLEND;
      break;
    case 6:
      if (memcmp(value, _s_gltf_OPAQUE, 6) == 0)
        return AK_OPAQUE_OPAQUE;
      break;
    default:
      break;
  }

  return AK_OPAQUE_OPAQUE;
}

AK_HIDE AkInterpolationType
gltf_interp(const json_t * __restrict json) {
  AkEnum val;
  long   glenums_len;
  long   i;

  dae_enum glenums[] = {
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
