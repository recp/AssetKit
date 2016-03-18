/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_enums.h"
#include "../../ak_common.h"
#include <string.h>

typedef struct {
  const char * name;
  long         val;
} ak_dae_enum;

long _assetkit_hide
ak_dae_fxEnumGlFunc(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NEVER",    ak_GL_FUNC_NEVER},
    {"LESS",     ak_GL_FUNC_LESS},
    {"LEQUAL",   ak_GL_FUNC_LEQUAL},
    {"EQUAL",    ak_GL_FUNC_EQUAL},
    {"GREATER",  ak_GL_FUNC_GREATER},
    {"NOTEQUAL", ak_GL_FUNC_NOTEQUAL},
    {"GEQUAL",   ak_GL_FUNC_GEQUAL},
    {"ALWAYS",   ak_GL_FUNC_ALWAYS}
  };

  /* COLLADA 1.5: ALWAYS is the default */
  val = ak_GL_FUNC_ALWAYS;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumBlend(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"ZERO",                     ak_GL_BLEND_ZERO},
    {"ONE",                      ak_GL_BLEND_ONE},
    {"SRC_COLOR",                ak_GL_BLEND_SRC_COLOR},
    {"ONE_MINUS_SRC_COLOR",      ak_GL_BLEND_ONE_MINUS_SRC_COLOR},
    {"DEST_COLOR",               ak_GL_BLEND_DEST_COLOR},
    {"ONE_MINUS_DEST_COLOR",     ak_GL_BLEND_ONE_MINUS_DEST_COLOR},
    {"SRC_ALPHA",                ak_GL_BLEND_SRC_ALPHA},
    {"ONE_MINUS_SRC_ALPHA",      ak_GL_BLEND_ONE_MINUS_SRC_ALPHA},
    {"DST_ALPHA",                ak_GL_BLEND_DST_ALPHA},
    {"ONE_MINUS_DST_ALPHA",      ak_GL_BLEND_ONE_MINUS_DST_ALPHA},
    {"CONSTANT_COLOR",           ak_GL_BLEND_CONSTANT_COLOR},
    {"ONE_MINUS_CONSTANT_COLOR", ak_GL_BLEND_ONE_MINUS_CONSTANT_COLOR},
    {"CONSTANT_ALPHA",           ak_GL_BLEND_CONSTANT_ALPHA},
    {"ONE_MINUS_CONSTANT_ALPHA", ak_GL_BLEND_ONE_MINUS_CONSTANT_ALPHA},
    {"SRC_ALPHA_SATURATE",       ak_GL_BLEND_SRC_ALPHA_SATURATE}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumBlendEq(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FUNC_ADD",              ak_GL_BLEND_EQUATION_FUNC_ADD},
    {"FUNC_SUBTRACT",         ak_GL_BLEND_EQUATION_FUNC_SUBTRACT},
    {"FUNC_REVERSE_SUBTRACT", ak_GL_BLEND_EQUATION_FUNC_REVERSE_SUBTRACT},
    {"MIN",                   ak_GL_BLEND_EQUATION_MIN},
    {"MAX",                   ak_GL_BLEND_EQUATION_MAX}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumGLFace(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FRONT",          ak_GL_FACE_FRONT},
    {"BACK",           ak_GL_FACE_BACK},
    {"FRONT_AND_BACK", ak_GL_FACE_FRONT_AND_BACK}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumMaterial(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"EMISSION",            ak_GL_MATERIAL_EMISSION},
    {"AMBIENT",             ak_GL_MATERIAL_AMBIENT},
    {"DIFFUSE",             ak_GL_MATERIAL_DIFFUSE},
    {"SPECULAR",            ak_GL_MATERIAL_SPECULAR},
    {"AMBIENT_AND_DIFFUSE", ak_GL_MATERIAL_AMBIENT_AND_DIFFUSE},
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumFog(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"LINEAR", ak_GL_FOG_LINEAR},
    {"EXP",    ak_GL_FOG_EXP},
    {"EXP2",   ak_GL_FOG_EXP2}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumFogCoordSrc(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FOG_COORDINATE", ak_GL_FOG_COORD_SRC_FOG_COORDINATE},
    {"FRAGMENT_DEPTH", ak_GL_FOG_COORD_SRC_FRAGMENT_DEPTH}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumFrontFace(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"CW",  ak_GL_FRONT_FACE_CW},
    {"CCW", ak_GL_FRONT_FACE_CCW}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumLightModelColorCtl(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"SINGLE_COLOR",
      ak_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR},
    {"SEPARATE_SPECULAR_COLOR",
      ak_GL_LIGHT_MODEL_COLOR_CONTROL_SEPARATE_SPECULAR_COLOR}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumLogicOp(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"CLEAR",         ak_GL_LOGIC_OP_CLEAR},
    {"AND",           ak_GL_LOGIC_OP_AND},
    {"AND_REVERSE",   ak_GL_LOGIC_OP_AND_REVERSE},
    {"COPY",          ak_GL_LOGIC_OP_COPY},
    {"AND_INVERTED",  ak_GL_LOGIC_OP_AND_INVERTED},
    {"NOOP",          ak_GL_LOGIC_OP_NOOP},
    {"XOR",           ak_GL_LOGIC_OP_XOR},
    {"OR",            ak_GL_LOGIC_OP_OR},
    {"NOR",           ak_GL_LOGIC_OP_NOR},
    {"EQUIV",         ak_GL_LOGIC_OP_EQUIV},
    {"INVERT",        ak_GL_LOGIC_OP_INVERT},
    {"OR_REVERSE",    ak_GL_LOGIC_OP_OR_REVERSE},
    {"COPY_INVERTED", ak_GL_LOGIC_OP_COPY_INVERTED},
    {"NAND",          ak_GL_LOGIC_OP_NAND},
    {"SET",           ak_GL_LOGIC_OP_SET}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumPolyMode(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"POINT", ak_GL_POLYGON_MODE_POINT},
    {"LINE",  ak_GL_POLYGON_MODE_LINE},
    {"FILL",  ak_GL_POLYGON_MODE_FILL},
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumShadeModel(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FLAT",   ak_GL_SHADE_MODEL_FLAT},
    {"SMOOTH", ak_GL_SHADE_MODEL_SMOOTH}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumStencilOp(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"KEEP",      ak_GL_STENCIL_OP_KEEP},
    {"ZERO",      ak_GL_STENCIL_OP_ZERO},
    {"REPLACE",   ak_GL_STENCIL_OP_REPLACE},
    {"INCR",      ak_GL_STENCIL_OP_INCR},
    {"DECR",      ak_GL_STENCIL_OP_DECR},
    {"INVERT",    ak_GL_STENCIL_OP_INVERT},
    {"INCR_WRAP", ak_GL_STENCIL_OP_INCR_WRAP},
    {"DECR_WRAP", ak_GL_STENCIL_OP_DECR_WRAP}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumWrap(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"WRAP",        ak_WRAP_MODE_WRAP},
    {"CLAMP",       ak_WRAP_MODE_CLAMP},
    {"BORDER",      ak_WRAP_MODE_BORDER},
    {"MIRROR",      ak_WRAP_MODE_MIRROR},
    {"MIRROR_ONCE", ak_WRAP_MODE_MIRROR_ONCE}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumMinfilter(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NEAREST",     ak_MINFILTER_NEAREST},
    {"LINEAR",      ak_MINFILTER_LINEAR},
    {"ANISOTROPIC", ak_MINFILTER_ANISOTROPIC}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumMipfilter(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NONE",    ak_MIPFILTER_NONE},
    {"NEAREST", ak_MIPFILTER_NEAREST},
    {"LINEAR",  ak_MIPFILTER_LINEAR}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumMagfilter(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NEAREST", ak_MAGFILTER_NEAREST},
    {"LINEAR",  ak_MAGFILTER_LINEAR}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumShaderStage(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"VERTEX",      ak_PIPELINE_STAGE_VERTEX},
    {"FRAGMENT",    ak_PIPELINE_STAGE_FRAGMENT},
    {"TESSELATION", ak_PIPELINE_STAGE_TESSELATION},
    {"GEOMETRY",    ak_PIPELINE_STAGE_GEOMETRY}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumFace(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"POSITIVE_X", ak_FACE_POSITIVE_X},
    {"NEGATIVE_X", ak_FACE_NEGATIVE_X},
    {"POSITIVE_Y", ak_FACE_POSITIVE_Y},
    {"NEGATIVE_Y", ak_FACE_NEGATIVE_Y},
    {"POSITIVE_Z", ak_FACE_POSITIVE_Z},
    {"NEGATIVE_Z", ak_FACE_NEGATIVE_Z}
  };

  val = -1;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumDraw(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"GEOMETRY",         ak_DRAW_GEOMETRY},
    {"SCENE_GEOMETRY",   ak_DRAW_SCENE_GEOMETRY},
    {"SCENE_IMAGE",      ak_DRAW_SCENE_IMAGE},
    {"FULL_SCREEN_QUAD", ak_DRAW_FULL_SCREEN_QUAD},
    {"FULL_SCREEN_QUAD_PLUS_HALF_PIXEL",
      ak_DRAW_FULL_SCREEN_QUAD_PLUS_HALF_PIXEL}
  };

  /* ak_DRAW_READ_STR_VAL */
  val = 0;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumOpaque(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"A_ONE",    ak_OPAQUE_A_ONE},
    {"RGB_ZERO", ak_OPAQUE_RGB_ZERO},
    {"A_ZERO",   ak_OPAQUE_A_ZERO},
    {"RGB_ONE",  ak_OPAQUE_RGB_ONE}
  };

  val = 0;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumChannel(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"RGB",  ak_FORMAT_CHANNEL_RGB},
    {"RGBA", ak_FORMAT_CHANNEL_RGBA},
    {"RGBE", ak_FORMAT_CHANNEL_RGBE},
    {"L",    ak_FORMAT_CHANNEL_L},
    {"LA",   ak_FORMAT_CHANNEL_LA},
    {"D",    ak_FORMAT_CHANNEL_D}
  };

  val = 0;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumRange(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"SNORM", ak_FORMAT_RANGE_SNORM},
    {"UNORM", ak_FORMAT_RANGE_UNORM},
    {"SINT",  ak_FORMAT_RANGE_SINT},
    {"UINT",  ak_FORMAT_RANGE_UINT},
    {"FLOAT", ak_FORMAT_RANGE_FLOAT}
  };

  val = 0;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetkit_hide
ak_dae_fxEnumPrecision(const char * name) {
  long val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"DEFAULT", ak_FORMAT_PRECISION_DEFAULT},
    {"LOW",     ak_FORMAT_PRECISION_LOW},
    {"MID",     ak_FORMAT_PRECISION_MID},
    {"HIGH",    ak_FORMAT_PRECISION_HIGHT},
    {"MAX",     ak_FORMAT_PRECISION_MAX}
  };

  val = 0;
  glenums_len = ak_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}
