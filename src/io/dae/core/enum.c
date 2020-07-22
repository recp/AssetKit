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
#include "../common.h"
#include <string.h>

AkEnum AK_HIDE
dae_semantic(const char * name) {
  AkEnum val;
  long   glenums_len, i;

  if (!name)
    return AK_INPUT_OTHER;
  
  dae_enum glenums[] = {
    {"BINORMAL",        AK_INPUT_BINORMAL},
    {"COLOR",           AK_INPUT_COLOR},
    {"CONTINUITY",      AK_INPUT_CONTINUITY},
    {"IMAGE",           AK_INPUT_IMAGE},
    {"INPUT",           AK_INPUT_INPUT},
    {"IN_TANGENT",      AK_INPUT_IN_TANGENT},
    {"INTERPOLATION",   AK_INPUT_INTERPOLATION},
    {"INV_BIND_MATRIX", AK_INPUT_INV_BIND_MATRIX},
    {"JOINT",           AK_INPUT_JOINT},
    {"LINEAR_STEPS",    AK_INPUT_LINEAR_STEPS},
    {"MORPH_TARGET",    AK_INPUT_MORPH_TARGET},
    {"MORPH_WEIGHT",    AK_INPUT_MORPH_WEIGHT},
    {"NORMAL",          AK_INPUT_NORMAL},
    {"OUTPUT",          AK_INPUT_OUTPUT},
    {"OUT_TANGENT",     AK_INPUT_OUT_TANGENT},
    {"POSITION",        AK_INPUT_POSITION},
    {"TANGENT",         AK_INPUT_TANGENT},
    {"TEXBINORMAL",     AK_INPUT_TEXBINORMAL},
    {"TEXCOORD",        AK_INPUT_TEXCOORD},
    {"TEXTANGENT",      AK_INPUT_TEXTANGENT},
    {"UV",              AK_INPUT_UV},
    {"VERTEX",          AK_INPUT_SEMANTIC_VERTEX},
    {"WEIGHT",          AK_INPUT_WEIGHT},
  };

  /* COLLADA 1.5: ALWAYS is the default */
  val         = AK_INPUT_OTHER;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_morphMethod(const xml_attr_t * __restrict xatt) {
  AkEnum val;
  long   glenums_len, i;

  if (!xatt)
    return AK_MORPH_METHOD_NORMALIZED;
  
  dae_enum glenums[] = {
    {"NORMALIZED", AK_MORPH_METHOD_NORMALIZED},
    {"RELATIVE",   AK_MORPH_METHOD_RELATIVE},
  };

  val         = AK_MORPH_METHOD_NORMALIZED;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(xatt->val, glenums[i].name, xatt->valsize) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_nodeType(const xml_attr_t * __restrict xatt) {
  AkEnum val;
  long   glenums_len, i;
  
  if (!xatt)
    return AK_NODE_TYPE_NODE;

  dae_enum glenums[] = {
    {"NODE",  AK_NODE_TYPE_NODE},
    {"JOINT", AK_NODE_TYPE_JOINT},
  };

  val         = AK_NODE_TYPE_NODE;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(xatt->val, glenums[i].name, xatt->valsize) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_animBehavior(const xml_attr_t * __restrict xatt) {
  AkEnum val;
  long   glenums_len, i;

  if (!xatt)
    return AK_SAMPLER_BEHAVIOR_UNDEFINED;
  
  dae_enum glenums[] = {
    {"UNDEFINED",      AK_SAMPLER_BEHAVIOR_UNDEFINED},
    {"CONSTANT",       AK_SAMPLER_BEHAVIOR_CONSTANT},
    {"GRADIENT",       AK_SAMPLER_BEHAVIOR_GRADIENT},
    {"CYCLE",          AK_SAMPLER_BEHAVIOR_CYCLE},
    {"OSCILLATE",      AK_SAMPLER_BEHAVIOR_OSCILLATE},
    {"CYCLE_RELATIVE", AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE}
  };

  val         = AK_SAMPLER_BEHAVIOR_UNDEFINED;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(xatt->val, glenums[i].name, xatt->valsize) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_animInterp(const char *name, size_t len) {
  AkEnum val;
  long   glenums_len, i;

  if (!name)
    return AK_INTERPOLATION_LINEAR;
  
  dae_enum glenums[] = {
    {"LINEAR",   AK_INTERPOLATION_LINEAR},
    {"BEZIER",   AK_INTERPOLATION_BEZIER},
    {"CARDINAL", AK_INTERPOLATION_CARDINAL},
    {"HERMITE",  AK_INTERPOLATION_HERMITE},
    {"BSPLINE",  AK_INTERPOLATION_BSPLINE},
    {"STEP",     AK_INTERPOLATION_STEP}
  };

  val         = AK_INTERPOLATION_LINEAR;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(name, glenums[i].name, len) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_wrap(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  dae_enum glenums[] = {
    {"WRAP",        AK_WRAP_MODE_WRAP},
    {"CLAMP",       AK_WRAP_MODE_CLAMP},
    {"BORDER",      AK_WRAP_MODE_BORDER},
    {"MIRROR",      AK_WRAP_MODE_MIRROR},
    {"MIRROR_ONCE", AK_WRAP_MODE_MIRROR_ONCE}
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

AkEnum AK_HIDE
dae_minfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  dae_enum glenums[] = {
    {"NEAREST",     AK_MINFILTER_NEAREST},
    {"LINEAR",      AK_MINFILTER_LINEAR},
    {"ANISOTROPIC", AK_MINFILTER_ANISOTROPIC}
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

AkEnum AK_HIDE
dae_mipfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  dae_enum glenums[] = {
    {"NONE",    AK_MIPFILTER_NONE},
    {"NEAREST", AK_MIPFILTER_NEAREST},
    {"LINEAR",  AK_MIPFILTER_LINEAR}
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

AkEnum AK_HIDE
dae_magfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

  if (!xml)
    return 0;
  
  dae_enum glenums[] = {
    {"NEAREST", AK_MAGFILTER_NEAREST},
    {"LINEAR",  AK_MAGFILTER_LINEAR}
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

AkEnum AK_HIDE
dae_face(const xml_attr_t * __restrict xatt) {
  AkEnum val;
  long   glenums_len, i;

  if (!xatt)
    return 0;
  
  dae_enum glenums[] = {
    {"POSITIVE_X", AK_FACE_POSITIVE_X},
    {"NEGATIVE_X", AK_FACE_NEGATIVE_X},
    {"POSITIVE_Y", AK_FACE_POSITIVE_Y},
    {"NEGATIVE_Y", AK_FACE_NEGATIVE_Y},
    {"POSITIVE_Z", AK_FACE_POSITIVE_Z},
    {"NEGATIVE_Z", AK_FACE_NEGATIVE_Z}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(xatt->val, glenums[i].name, xatt->valsize) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_opaque(const xml_attr_t * __restrict xatt) {
  AkEnum val;
  long  glenums_len, i;

  if (!xatt)
    return AK_OPAQUE_A_ONE;
  
  dae_enum glenums[] = {
    {"A_ONE",    AK_OPAQUE_A_ONE},
    {"RGB_ZERO", AK_OPAQUE_RGB_ZERO},
    {"A_ZERO",   AK_OPAQUE_A_ZERO},
    {"RGB_ONE",  AK_OPAQUE_RGB_ONE}
  };

  val         = AK_OPAQUE_A_ONE;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(xatt->val, glenums[i].name, xatt->valsize) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_enumChannel(const char *name, size_t len) {
  AkEnum val;
  long   glenums_len, i;

  if (!name)
    return 0;
  
  dae_enum glenums[] = {
    {"RGB",  AK_CHANNEL_FORMAT_RGB},
    {"RGBA", AK_CHANNEL_FORMAT_RGBA},
    {"RGBE", AK_CHANNEL_FORMAT_RGBE},
    {"L",    AK_CHANNEL_FORMAT_L},
    {"LA",   AK_CHANNEL_FORMAT_LA},
    {"D",    AK_CHANNEL_FORMAT_D},

    /* 1.4 */
    {"XYZ",  AK_CHANNEL_FORMAT_XYZ},
    {"XYZW", AK_CHANNEL_FORMAT_XYZW}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(name, glenums[i].name, len) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_range(const char *name, size_t len) {
  AkEnum val;
  long   glenums_len, i;

  if (!name)
    return 0;
  
  dae_enum glenums[] = {
    {"SNORM", AK_RANGE_FORMAT_SNORM},
    {"UNORM", AK_RANGE_FORMAT_UNORM},
    {"SINT",  AK_RANGE_FORMAT_SINT},
    {"UINT",  AK_RANGE_FORMAT_UINT},
    {"FLOAT", AK_RANGE_FORMAT_FLOAT}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(name, glenums[i].name, len) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum AK_HIDE
dae_precision(const char *name, size_t len) {
  AkEnum val;
  long   glenums_len, i;

  if (!name)
    return 0;

  dae_enum glenums[] = {
    {"DEFAULT", AK_PRECISION_FORMAT_DEFAULT},
    {"LOW",     AK_PRECISION_FORMAT_LOW},
    {"MID",     AK_PRECISION_FORMAT_MID},
    {"HIGH",    AK_PRECISION_FORMAT_HIGHT},
    {"MAX",     AK_PRECISION_FORMAT_MAX}
  };

  val         = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(name, glenums[i].name, len) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}
