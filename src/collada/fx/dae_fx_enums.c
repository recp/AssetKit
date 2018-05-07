/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_enums.h"
#include "../../common.h"
#include <string.h>

AkEnum _assetkit_hide
ak_dae_fxEnumGlFunc(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NEVER",    AK_GL_FUNC_NEVER},
    {"LESS",     AK_GL_FUNC_LESS},
    {"LEQUAL",   AK_GL_FUNC_LEQUAL},
    {"EQUAL",    AK_GL_FUNC_EQUAL},
    {"GREATER",  AK_GL_FUNC_GREATER},
    {"NOTEQUAL", AK_GL_FUNC_NOTEQUAL},
    {"GEQUAL",   AK_GL_FUNC_GEQUAL},
    {"ALWAYS",   AK_GL_FUNC_ALWAYS}
  };

  /* COLLADA 1.5: ALWAYS is the default */
  val = AK_GL_FUNC_ALWAYS;
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
ak_dae_fxEnumBlend(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"ZERO",                     AK_GL_BLEND_ZERO},
    {"ONE",                      AK_GL_BLEND_ONE},
    {"SRC_COLOR",                AK_GL_BLEND_SRC_COLOR},
    {"ONE_MINUS_SRC_COLOR",      AK_GL_BLEND_ONE_MINUS_SRC_COLOR},
    {"DEST_COLOR",               AK_GL_BLEND_DEST_COLOR},
    {"ONE_MINUS_DEST_COLOR",     AK_GL_BLEND_ONE_MINUS_DEST_COLOR},
    {"SRC_ALPHA",                AK_GL_BLEND_SRC_ALPHA},
    {"ONE_MINUS_SRC_ALPHA",      AK_GL_BLEND_ONE_MINUS_SRC_ALPHA},
    {"DST_ALPHA",                AK_GL_BLEND_DST_ALPHA},
    {"ONE_MINUS_DST_ALPHA",      AK_GL_BLEND_ONE_MINUS_DST_ALPHA},
    {"CONSTANT_COLOR",           AK_GL_BLEND_CONSTANT_COLOR},
    {"ONE_MINUS_CONSTANT_COLOR", AK_GL_BLEND_ONE_MINUS_CONSTANT_COLOR},
    {"CONSTANT_ALPHA",           AK_GL_BLEND_CONSTANT_ALPHA},
    {"ONE_MINUS_CONSTANT_ALPHA", AK_GL_BLEND_ONE_MINUS_CONSTANT_ALPHA},
    {"SRC_ALPHA_SATURATE",       AK_GL_BLEND_SRC_ALPHA_SATURATE}
  };

  val = -1;
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
ak_dae_fxEnumBlendEq(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FUNC_ADD",              AK_GL_BLEND_EQUATION_FUNC_ADD},
    {"FUNC_SUBTRACT",         AK_GL_BLEND_EQUATION_FUNC_SUBTRACT},
    {"FUNC_REVERSE_SUBTRACT", AK_GL_BLEND_EQUATION_FUNC_REVERSE_SUBTRACT},
    {"MIN",                   AK_GL_BLEND_EQUATION_MIN},
    {"MAX",                   AK_GL_BLEND_EQUATION_MAX}
  };

  val = -1;
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
ak_dae_fxEnumGLFace(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FRONT",          AK_GL_FACE_FRONT},
    {"BACK",           AK_GL_FACE_BACK},
    {"FRONT_AND_BACK", AK_GL_FACE_FRONT_AND_BACK}
  };

  val = -1;
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
ak_dae_fxEnumMaterial(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"EMISSION",            AK_GL_MATERIAL_TYPE_EMISSION},
    {"AMBIENT",             AK_GL_MATERIAL_TYPE_AMBIENT},
    {"DIFFUSE",             AK_GL_MATERIAL_TYPE_DIFFUSE},
    {"SPECULAR",            AK_GL_MATERIAL_TYPE_SPECULAR},
    {"AMBIENT_AND_DIFFUSE", AK_GL_MATERIAL_TYPE_AMBIENT_AND_DIFFUSE},
  };

  val = -1;
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
ak_dae_fxEnumFog(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"LINEAR", AK_GL_FOG_LINEAR},
    {"EXP",    AK_GL_FOG_EXP},
    {"EXP2",   AK_GL_FOG_EXP2}
  };

  val = -1;
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
ak_dae_fxEnumFogCoordSrc(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FOG_COORDINATE", AK_GL_FOG_COORD_SRC_FOG_COORDINATE},
    {"FRAGMENT_DEPTH", AK_GL_FOG_COORD_SRC_FRAGMENT_DEPTH}
  };

  val = -1;
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
ak_dae_fxEnumFrontFace(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"CW",  AK_GL_FRONT_FACE_CW},
    {"CCW", AK_GL_FRONT_FACE_CCW}
  };

  val = -1;
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
ak_dae_fxEnumLightModelColorCtl(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"SINGLE_COLOR",
      AK_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR},
    {"SEPARATE_SPECULAR_COLOR",
      AK_GL_LIGHT_MODEL_COLOR_CONTROL_SEPARATE_SPECULAR_COLOR}
  };

  val = -1;
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
ak_dae_fxEnumLogicOp(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"CLEAR",         AK_GL_LOGIC_OP_CLEAR},
    {"AND",           AK_GL_LOGIC_OP_AND},
    {"AND_REVERSE",   AK_GL_LOGIC_OP_AND_REVERSE},
    {"COPY",          AK_GL_LOGIC_OP_COPY},
    {"AND_INVERTED",  AK_GL_LOGIC_OP_AND_INVERTED},
    {"NOOP",          AK_GL_LOGIC_OP_NOOP},
    {"XOR",           AK_GL_LOGIC_OP_XOR},
    {"OR",            AK_GL_LOGIC_OP_OR},
    {"NOR",           AK_GL_LOGIC_OP_NOR},
    {"EQUIV",         AK_GL_LOGIC_OP_EQUIV},
    {"INVERT",        AK_GL_LOGIC_OP_INVERT},
    {"OR_REVERSE",    AK_GL_LOGIC_OP_OR_REVERSE},
    {"COPY_INVERTED", AK_GL_LOGIC_OP_COPY_INVERTED},
    {"NAND",          AK_GL_LOGIC_OP_NAND},
    {"SET",           AK_GL_LOGIC_OP_SET}
  };

  val = -1;
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
ak_dae_fxEnumPolyMode(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"POINT", AK_GL_POLYGON_MODE_POINT},
    {"LINE",  AK_GL_POLYGON_MODE_LINE},
    {"FILL",  AK_GL_POLYGON_MODE_FILL},
  };

  val = -1;
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
ak_dae_fxEnumShadeModel(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"FLAT",   AK_GL_SHADE_MODEL_FLAT},
    {"SMOOTH", AK_GL_SHADE_MODEL_SMOOTH}
  };

  val = -1;
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
ak_dae_fxEnumStencilOp(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"KEEP",      AK_GL_STENCIL_OP_KEEP},
    {"ZERO",      AK_GL_STENCIL_OP_ZERO},
    {"REPLACE",   AK_GL_STENCIL_OP_REPLACE},
    {"INCR",      AK_GL_STENCIL_OP_INCR},
    {"DECR",      AK_GL_STENCIL_OP_DECR},
    {"INVERT",    AK_GL_STENCIL_OP_INVERT},
    {"INCR_WRAP", AK_GL_STENCIL_OP_INCR_WRAP},
    {"DECR_WRAP", AK_GL_STENCIL_OP_DECR_WRAP}
  };

  val = -1;
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
ak_dae_fxEnumWrap(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"WRAP",        AK_WRAP_MODE_WRAP},
    {"CLAMP",       AK_WRAP_MODE_CLAMP},
    {"BORDER",      AK_WRAP_MODE_BORDER},
    {"MIRROR",      AK_WRAP_MODE_MIRROR},
    {"MIRROR_ONCE", AK_WRAP_MODE_MIRROR_ONCE}
  };

  val = -1;
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
ak_dae_fxEnumMinfilter(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NEAREST",     AK_MINFILTER_NEAREST},
    {"LINEAR",      AK_MINFILTER_LINEAR},
    {"ANISOTROPIC", AK_MINFILTER_ANISOTROPIC}
  };

  val = -1;
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
ak_dae_fxEnumMipfilter(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NONE",    AK_MIPFILTER_NONE},
    {"NEAREST", AK_MIPFILTER_NEAREST},
    {"LINEAR",  AK_MIPFILTER_LINEAR}
  };

  val = -1;
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
ak_dae_fxEnumMagfilter(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"NEAREST", AK_MAGFILTER_NEAREST},
    {"LINEAR",  AK_MAGFILTER_LINEAR}
  };

  val = -1;
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
ak_dae_fxEnumShaderStage(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"VERTEX",      AK_PIPELINE_STAGE_VERTEX},
    {"FRAGMENT",    AK_PIPELINE_STAGE_FRAGMENT},
    {"TESSELATION", AK_PIPELINE_STAGE_TESSELATION},
    {"GEOMETRY",    AK_PIPELINE_STAGE_GEOMETRY}
  };

  val = -1;
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
ak_dae_fxEnumFace(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"POSITIVE_X", AK_FACE_POSITIVE_X},
    {"NEGATIVE_X", AK_FACE_NEGATIVE_X},
    {"POSITIVE_Y", AK_FACE_POSITIVE_Y},
    {"NEGATIVE_Y", AK_FACE_NEGATIVE_Y},
    {"POSITIVE_Z", AK_FACE_POSITIVE_Z},
    {"NEGATIVE_Z", AK_FACE_NEGATIVE_Z}
  };

  val = -1;
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
ak_dae_fxEnumDraw(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"GEOMETRY",         AK_DRAW_GEOMETRY},
    {"SCENE_GEOMETRY",   AK_DRAW_SCENE_GEOMETRY},
    {"SCENE_IMAGE",      AK_DRAW_SCENE_IMAGE},
    {"FULL_SCREEN_QUAD", AK_DRAW_FULL_SCREEN_QUAD},
    {"FULL_SCREEN_QUAD_PLUS_HALF_PIXEL",
      AK_DRAW_FULL_SCREEN_QUAD_PLUS_HALF_PIXEL}
  };

  /* AK_DRAW_READ_STR_VAL */
  val = 0;
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
ak_dae_fxEnumOpaque(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"A_ONE",    AK_OPAQUE_A_ONE},
    {"RGB_ZERO", AK_OPAQUE_RGB_ZERO},
    {"A_ZERO",   AK_OPAQUE_A_ZERO},
    {"RGB_ONE",  AK_OPAQUE_RGB_ONE}
  };

  val = AK_OPAQUE_A_ONE;
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
ak_dae_fxEnumChannel(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
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

  val = 0;
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
ak_dae_fxEnumRange(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"SNORM", AK_RANGE_FORMAT_SNORM},
    {"UNORM", AK_RANGE_FORMAT_UNORM},
    {"SINT",  AK_RANGE_FORMAT_SINT},
    {"UINT",  AK_RANGE_FORMAT_UINT},
    {"FLOAT", AK_RANGE_FORMAT_FLOAT}
  };

  val = 0;
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
ak_dae_fxEnumPrecision(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"DEFAULT", AK_PRECISION_FORMAT_DEFAULT},
    {"LOW",     AK_PRECISION_FORMAT_LOW},
    {"MID",     AK_PRECISION_FORMAT_MID},
    {"HIGH",    AK_PRECISION_FORMAT_HIGHT},
    {"MAX",     AK_PRECISION_FORMAT_MAX}
  };

  val = 0;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}
