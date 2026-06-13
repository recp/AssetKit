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
#include "../../../common.h"
#include "../../../string_fast.h"
#include "../common.h"
#include <string.h>

typedef struct dae_fast_enum {
  const char *name;
  size_t      len;
  uint64_t    packed;
  AkEnum      val;
} dae_fast_enum;

#define DAE_FAST_ENUM_SHORT(NAME, VAL)                                       \
  {_s_dae_##NAME, _s_dae_##NAME##_len, _s_dae_##NAME##_u64, VAL}
#define DAE_FAST_ENUM_LONG(NAME, VAL)                                        \
  {_s_dae_##NAME, _s_dae_##NAME##_len, 0, VAL}

#define DAE_SEMANTIC_SHORT(NAME, VAL)                                        \
  do {                                                                       \
    if (ak_str_eq_packed_ci_fast(name, len,                                  \
                                 _s_dae_##NAME##_u64,                       \
                                 _s_dae_##NAME##_len)) {                    \
      if (semantic)                                                          \
        *semantic = VAL;                                                     \
      return _s_dae_##NAME;                                                  \
    }                                                                        \
  } while (0)

#define DAE_SEMANTIC_LONG(NAME, VAL)                                         \
  do {                                                                       \
    if (ak_str_eq_ci_len_fast(name, len,                                     \
                              _s_dae_##NAME, _s_dae_##NAME##_len)) {        \
      if (semantic)                                                          \
        *semantic = VAL;                                                     \
      return _s_dae_##NAME;                                                  \
    }                                                                        \
  } while (0)

static inline
bool
dae_enumEqCi(const char              * __restrict name,
             size_t                               len,
             const dae_fast_enum     * __restrict item) {
  if (!name || len != item->len)
    return false;

  if (len <= 8)
    return ak_str_eq_packed_ci_fast(name, len, item->packed, item->len);

  return ak_str_eq_ci_fast(name, item->name, item->len);
}

static inline
AkEnum
dae_enumLookupCi(const char              * __restrict name,
                 size_t                               len,
                 const dae_fast_enum     * __restrict items,
                 size_t                               count,
                 AkEnum                               fallback) {
  size_t i;

  for (i = 0; i < count; i++) {
    if (dae_enumEqCi(name, len, &items[i]))
      return items[i].val;
  }

  return fallback;
}

static inline
AkEnum
dae_attrEnumLookupCi(const xml_attr_t        * __restrict xatt,
                     const dae_fast_enum     * __restrict items,
                     size_t                               count,
                     AkEnum                               fallback) {
  if (!xatt)
    return fallback;

  return dae_enumLookupCi(xatt->val,
                          xatt->valsize,
                          items,
                          count,
                          fallback);
}

AK_HIDE const char*
dae_semanticRaw(const xml_attr_t * __restrict xatt,
                AkHeap          * __restrict heap,
                void            * __restrict parent,
                AkInputSemantic * __restrict semantic) {
  const char *name;
  size_t      len;

  if (semantic)
    *semantic = AK_INPUT_OTHER;

  if (!xatt || !(name = xatt->val))
    return NULL;

  len = xatt->valsize;

  switch (len) {
    case 2:
      DAE_SEMANTIC_SHORT(UV, AK_INPUT_UV);
      break;
    case 5:
      DAE_SEMANTIC_SHORT(COLOR, AK_INPUT_COLOR);
      DAE_SEMANTIC_SHORT(IMAGE, AK_INPUT_IMAGE);
      DAE_SEMANTIC_SHORT(INPUT, AK_INPUT_INPUT);
      DAE_SEMANTIC_SHORT(JOINT, AK_INPUT_JOINT);
      break;
    case 6:
      DAE_SEMANTIC_SHORT(NORMAL, AK_INPUT_NORMAL);
      DAE_SEMANTIC_SHORT(OUTPUT, AK_INPUT_OUTPUT);
      DAE_SEMANTIC_SHORT(VERTEX, AK_INPUT_SEMANTIC_VERTEX);
      DAE_SEMANTIC_SHORT(WEIGHT, AK_INPUT_WEIGHT);
      break;
    case 7:
      DAE_SEMANTIC_SHORT(TANGENT, AK_INPUT_TANGENT);
      break;
    case 8:
      DAE_SEMANTIC_SHORT(BINORMAL, AK_INPUT_BINORMAL);
      DAE_SEMANTIC_SHORT(POSITION, AK_INPUT_POSITION);
      DAE_SEMANTIC_SHORT(TEXCOORD, AK_INPUT_TEXCOORD);
      break;
    case 10:
      DAE_SEMANTIC_LONG(CONTINUITY, AK_INPUT_CONTINUITY);
      DAE_SEMANTIC_LONG(IN_TANGENT, AK_INPUT_IN_TANGENT);
      DAE_SEMANTIC_LONG(TEXTANGENT, AK_INPUT_TEXTANGENT);
      break;
    case 11:
      DAE_SEMANTIC_LONG(OUT_TANGENT, AK_INPUT_OUT_TANGENT);
      DAE_SEMANTIC_LONG(TEXBINORMAL, AK_INPUT_TEXBINORMAL);
      break;
    case 12:
      DAE_SEMANTIC_LONG(MORPH_TARGET, AK_INPUT_MORPH_TARGET);
      DAE_SEMANTIC_LONG(MORPH_WEIGHT, AK_INPUT_MORPH_WEIGHT);
      DAE_SEMANTIC_LONG(LINEAR_STEPS, AK_INPUT_LINEAR_STEPS);
      break;
    case 13:
      DAE_SEMANTIC_LONG(INTERPOLATION, AK_INPUT_INTERPOLATION);
      break;
    case 15:
      DAE_SEMANTIC_LONG(INV_BIND_MATRIX, AK_INPUT_INV_BIND_MATRIX);
      break;
    default:
      break;
  }

  return xmla_strdup(xatt, heap, parent);
}

AK_HIDE AkEnum
dae_morphMethod(const xml_attr_t * __restrict xatt) {
  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_LONG(NORMALIZED, AK_MORPH_METHOD_NORMALIZED),
    DAE_FAST_ENUM_SHORT(RELATIVE,  AK_MORPH_METHOD_RELATIVE),
  };

  return dae_attrEnumLookupCi(xatt,
                              glenums,
                              AK_ARRAY_LEN(glenums),
                              AK_MORPH_METHOD_NORMALIZED);
}

AK_HIDE AkEnum
dae_nodeType(const xml_attr_t * __restrict xatt) {
  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_SHORT(NODE,  AK_NODE_TYPE_NODE),
    DAE_FAST_ENUM_SHORT(JOINT, AK_NODE_TYPE_JOINT),
  };

  return dae_attrEnumLookupCi(xatt,
                              glenums,
                              AK_ARRAY_LEN(glenums),
                              AK_NODE_TYPE_NODE);
}

AK_HIDE AkEnum
dae_animBehavior(const xml_attr_t * __restrict xatt) {
  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_LONG(UNDEFINED,      AK_SAMPLER_BEHAVIOR_UNDEFINED),
    DAE_FAST_ENUM_SHORT(CONSTANT,      AK_SAMPLER_BEHAVIOR_CONSTANT),
    DAE_FAST_ENUM_SHORT(GRADIENT,      AK_SAMPLER_BEHAVIOR_GRADIENT),
    DAE_FAST_ENUM_SHORT(CYCLE,         AK_SAMPLER_BEHAVIOR_CYCLE),
    DAE_FAST_ENUM_LONG(OSCILLATE,      AK_SAMPLER_BEHAVIOR_OSCILLATE),
    DAE_FAST_ENUM_LONG(CYCLE_RELATIVE, AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE)
  };

  return dae_attrEnumLookupCi(xatt,
                              glenums,
                              AK_ARRAY_LEN(glenums),
                              AK_SAMPLER_BEHAVIOR_UNDEFINED);
}

AK_HIDE AkEnum
dae_animInterp(const char *name, size_t len) {
  if (!name)
    return AK_INTERPOLATION_LINEAR;

  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_SHORT(LINEAR,   AK_INTERPOLATION_LINEAR),
    DAE_FAST_ENUM_SHORT(BEZIER,   AK_INTERPOLATION_BEZIER),
    DAE_FAST_ENUM_SHORT(CARDINAL, AK_INTERPOLATION_CARDINAL),
    DAE_FAST_ENUM_SHORT(HERMITE,  AK_INTERPOLATION_HERMITE),
    DAE_FAST_ENUM_SHORT(BSPLINE,  AK_INTERPOLATION_BSPLINE),
    DAE_FAST_ENUM_SHORT(STEP,     AK_INTERPOLATION_STEP)
  };

  return dae_enumLookupCi(name,
                          len,
                          glenums,
                          AK_ARRAY_LEN(glenums),
                          AK_INTERPOLATION_LINEAR);
}

AK_HIDE AkEnum
dae_wrap(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  static const dae_enum glenums[] = {
    {_s_dae_WRAP,        AK_WRAP_MODE_WRAP},
    {_s_dae_CLAMP,       AK_WRAP_MODE_CLAMP},
    {_s_dae_BORDER,      AK_WRAP_MODE_BORDER},
    {_s_dae_MIRROR,      AK_WRAP_MODE_MIRROR},
    {_s_dae_MIRROR_ONCE, AK_WRAP_MODE_MIRROR_ONCE}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (xml_val_eq(xml, glenums[i].name)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AK_HIDE AkEnum
dae_minfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  static const dae_enum glenums[] = {
    {_s_dae_NEAREST,     AK_MINFILTER_NEAREST},
    {_s_dae_LINEAR,      AK_MINFILTER_LINEAR},
    {_s_dae_ANISOTROPIC, AK_MINFILTER_ANISOTROPIC}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (xml_val_eq(xml, glenums[i].name)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AK_HIDE AkEnum
dae_mipfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  static const dae_enum glenums[] = {
    {_s_dae_NONE,    AK_MIPFILTER_NONE},
    {_s_dae_NEAREST, AK_MIPFILTER_NEAREST},
    {_s_dae_LINEAR,  AK_MIPFILTER_LINEAR}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (xml_val_eq(xml, glenums[i].name)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AK_HIDE AkEnum
dae_magfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  static const dae_enum glenums[] = {
    {_s_dae_NEAREST, AK_MAGFILTER_NEAREST},
    {_s_dae_LINEAR,  AK_MAGFILTER_LINEAR}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (xml_val_eq(xml, glenums[i].name)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AK_HIDE AkEnum
dae_face(const xml_attr_t * __restrict xatt) {
  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_LONG(POSITIVE_X, AK_FACE_POSITIVE_X),
    DAE_FAST_ENUM_LONG(NEGATIVE_X, AK_FACE_NEGATIVE_X),
    DAE_FAST_ENUM_LONG(POSITIVE_Y, AK_FACE_POSITIVE_Y),
    DAE_FAST_ENUM_LONG(NEGATIVE_Y, AK_FACE_NEGATIVE_Y),
    DAE_FAST_ENUM_LONG(POSITIVE_Z, AK_FACE_POSITIVE_Z),
    DAE_FAST_ENUM_LONG(NEGATIVE_Z, AK_FACE_NEGATIVE_Z)
  };

  return dae_attrEnumLookupCi(xatt, glenums, AK_ARRAY_LEN(glenums), 0);
}

AK_HIDE AkEnum
dae_opaque(const xml_attr_t * __restrict xatt) {
  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_SHORT(A_ONE,    AK_OPAQUE_A_ONE),
    DAE_FAST_ENUM_SHORT(RGB_ZERO, AK_OPAQUE_RGB_ZERO),
    DAE_FAST_ENUM_SHORT(A_ZERO,   AK_OPAQUE_A_ZERO),
    DAE_FAST_ENUM_SHORT(RGB_ONE,  AK_OPAQUE_RGB_ONE)
  };

  return dae_attrEnumLookupCi(xatt, glenums, AK_ARRAY_LEN(glenums), AK_OPAQUE_A_ONE);
}

AK_HIDE AkEnum
dae_enumChannel(const char *name, size_t len) {
  if (!name)
    return 0;

  if (len <= 4) {
    switch (ak_str_pack4_ci_fast(name, len)) {
      case _s_dae_RGB_u32:  return AK_CHANNEL_FORMAT_RGB;
      case _s_dae_RGBA_u32: return AK_CHANNEL_FORMAT_RGBA;
      case _s_dae_RGBE_u32: return AK_CHANNEL_FORMAT_RGBE;
      case _s_dae_L_u32:    return AK_CHANNEL_FORMAT_L;
      case _s_dae_LA_u32:   return AK_CHANNEL_FORMAT_LA;
      case _s_dae_D_u32:    return AK_CHANNEL_FORMAT_D;
      case _s_dae_XYZ_u32:  return AK_CHANNEL_FORMAT_XYZ;
      case _s_dae_XYZW_u32: return AK_CHANNEL_FORMAT_XYZW;
      default:              return 0;
    }
  }

  return 0;
}

AK_HIDE AkEnum
dae_range(const char *name, size_t len) {
  if (!name)
    return 0;

  if (len <= 4) {
    switch (ak_str_pack4_ci_fast(name, len)) {
      case _s_dae_SINT_u32: return AK_RANGE_FORMAT_SINT;
      case _s_dae_UINT_u32: return AK_RANGE_FORMAT_UINT;
      default:              return 0;
    }
  }

  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_SHORT(SNORM, AK_RANGE_FORMAT_SNORM),
    DAE_FAST_ENUM_SHORT(UNORM, AK_RANGE_FORMAT_UNORM),
    DAE_FAST_ENUM_SHORT(FLOAT, AK_RANGE_FORMAT_FLOAT)
  };

  return dae_enumLookupCi(name, len, glenums, AK_ARRAY_LEN(glenums), 0);
}

AK_HIDE AkEnum
dae_precision(const char *name, size_t len) {
  if (!name)
    return 0;

  if (len <= 4) {
    switch (ak_str_pack4_ci_fast(name, len)) {
      case _s_dae_LOW_u32:  return AK_PRECISION_FORMAT_LOW;
      case _s_dae_MID_u32:  return AK_PRECISION_FORMAT_MID;
      case _s_dae_HIGH_u32: return AK_PRECISION_FORMAT_HIGHT;
      case _s_dae_MAX_u32:  return AK_PRECISION_FORMAT_MAX;
      default:              return 0;
    }
  }

  static const dae_fast_enum glenums[] = {
    DAE_FAST_ENUM_SHORT(DEFAULT, AK_PRECISION_FORMAT_DEFAULT)
  };

  return dae_enumLookupCi(name, len, glenums, AK_ARRAY_LEN(glenums), 0);
}
