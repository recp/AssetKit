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

  dae_enum glenums[] = {
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

int _assetkit_hide
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

AkComponentSize _assetkit_hide
gltf_type(const char *name, size_t len) {
  AkComponentSize val;
  long            glenums_len;
  long            i;

  dae_enum glenums[] = {
    {_s_gltf_SCALAR, AK_COMPONENT_SIZE_SCALAR},
    {_s_gltf_VEC2,   AK_COMPONENT_SIZE_VEC2},
    {_s_gltf_VEC3,   AK_COMPONENT_SIZE_VEC3},
    {_s_gltf_VEC4,   AK_COMPONENT_SIZE_VEC4},
    {_s_gltf_MAT2,   AK_COMPONENT_SIZE_MAT2},
    {_s_gltf_MAT3,   AK_COMPONENT_SIZE_MAT3},
    {_s_gltf_MAT4,   AK_COMPONENT_SIZE_MAT4},
  };

  val = AK_COMPONENT_SIZE_UNKNOWN;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(name, glenums[i].name, len) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum _assetkit_hide
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

AkEnum _assetkit_hide
gltf_magFilter(int type) {
  switch (type) {
    case 9728:  return AK_MINFILTER_NEAREST;   break;
    case 9729:  return AK_MINFILTER_LINEAR;    break;
    default: break;
  }
  return 0;
}

AkEnum _assetkit_hide
gltf_wrapMode(int type) {
  switch (type) {
    case 33071:  return AK_WRAP_MODE_CLAMP;       break;
    case 33648:  return AK_WRAP_MODE_MIRROR;      break;
    case 10497:  return AK_WRAP_MODE_WRAP;        break;
    default: break;
  }
  return AK_WRAP_MODE_WRAP;
}

AkOpaque _assetkit_hide
gltf_alphaMode(const char *name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
    {_s_gltf_OPAQUE, AK_OPAQUE_OPAQUE},
    {_s_gltf_MASK,   AK_OPAQUE_MASK},
    {_s_gltf_BLEND,  AK_OPAQUE_BLEND}
  };

  val = AK_OPAQUE_OPAQUE;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkInterpolationType _assetkit_hide
gltf_interp(json_t * __restrict json) {
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
