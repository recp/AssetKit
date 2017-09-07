/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_enums.h"

AkEnum _assetkit_hide
gltf_enumInputSemantic(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {_s_gltf_COLOR,     AK_INPUT_SEMANTIC_COLOR},
    {_s_gltf_JOINTS,    AK_INPUT_SEMANTIC_JOINT},
    {_s_gltf_NORMAL,    AK_INPUT_SEMANTIC_NORMAL},
    {_s_gltf_POSITION,  AK_INPUT_SEMANTIC_POSITION},
    {_s_gltf_TANGENT,   AK_INPUT_SEMANTIC_TANGENT},
    {_s_gltf_TEXCOORD,  AK_INPUT_SEMANTIC_TEXCOORD},
    {_s_gltf_WEIGHTS,   AK_INPUT_SEMANTIC_WEIGHT},
  };

  val = AK_INPUT_SEMANTIC_OTHER;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum _assetkit_hide
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

AkEnum _assetkit_hide
gltf_type(const char *name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {_s_gltf_SCALAR, AK_GLTF_SCALAR},
    {_s_gltf_VEC2,   AK_GLTF_VEC2},
    {_s_gltf_VEC3,   AK_GLTF_VEC3},
    {_s_gltf_VEC4,   AK_GLTF_VEC4},
    {_s_gltf_MAT2,   AK_GLTF_MAT2},
    {_s_gltf_MAT3,   AK_GLTF_MAT3},
    {_s_gltf_MAT4,   AK_GLTF_MAT4},
  };

  val = AK_GLTF_SCALAR;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}
