/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_enums.h"
#include "../../aio_common.h"
#include <string.h>

typedef struct {
  const char * name;
  long         val;
} aio_dae_enum;

long _assetio_hide
aio_dae_fxEnumGlFunc(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"NEVER",    AIO_GL_FUNC_NEVER},
    {"LESS",     AIO_GL_FUNC_LESS},
    {"LEQUAL",   AIO_GL_FUNC_LEQUAL},
    {"EQUAL",    AIO_GL_FUNC_EQUAL},
    {"GREATER",  AIO_GL_FUNC_GREATER},
    {"NOTEQUAL", AIO_GL_FUNC_NOTEQUAL},
    {"GEQUAL",   AIO_GL_FUNC_GEQUAL},
    {"ALWAYS",   AIO_GL_FUNC_ALWAYS}
  };

  /* COLLADA 1.5: ALWAYS is the default */
  val = AIO_GL_FUNC_ALWAYS;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumBlend(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"ZERO",                     AIO_GL_BLEND_ZERO},
    {"ONE",                      AIO_GL_BLEND_ONE},
    {"SRC_COLOR",                AIO_GL_BLEND_SRC_COLOR},
    {"ONE_MINUS_SRC_COLOR",      AIO_GL_BLEND_ONE_MINUS_SRC_COLOR},
    {"DEST_COLOR",               AIO_GL_BLEND_DEST_COLOR},
    {"ONE_MINUS_DEST_COLOR",     AIO_GL_BLEND_ONE_MINUS_DEST_COLOR},
    {"SRC_ALPHA",                AIO_GL_BLEND_SRC_ALPHA},
    {"ONE_MINUS_SRC_ALPHA",      AIO_GL_BLEND_ONE_MINUS_SRC_ALPHA},
    {"DST_ALPHA",                AIO_GL_BLEND_DST_ALPHA},
    {"ONE_MINUS_DST_ALPHA",      AIO_GL_BLEND_ONE_MINUS_DST_ALPHA},
    {"CONSTANT_COLOR",           AIO_GL_BLEND_CONSTANT_COLOR},
    {"ONE_MINUS_CONSTANT_COLOR", AIO_GL_BLEND_ONE_MINUS_CONSTANT_COLOR},
    {"CONSTANT_ALPHA",           AIO_GL_BLEND_CONSTANT_ALPHA},
    {"ONE_MINUS_CONSTANT_ALPHA", AIO_GL_BLEND_ONE_MINUS_CONSTANT_ALPHA},
    {"SRC_ALPHA_SATURATE",       AIO_GL_BLEND_SRC_ALPHA_SATURATE}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumBlendEq(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"FUNC_ADD",              AIO_GL_BLEND_EQUATION_FUNC_ADD},
    {"FUNC_SUBTRACT",         AIO_GL_BLEND_EQUATION_FUNC_SUBTRACT},
    {"FUNC_REVERSE_SUBTRACT", AIO_GL_BLEND_EQUATION_FUNC_REVERSE_SUBTRACT},
    {"MIN",                   AIO_GL_BLEND_EQUATION_MIN},
    {"MAX",                   AIO_GL_BLEND_EQUATION_MAX}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumGLFace(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"FRONT",          AIO_GL_FACE_FRONT},
    {"BACK",           AIO_GL_FACE_BACK},
    {"FRONT_AND_BACK", AIO_GL_FACE_FRONT_AND_BACK}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumMaterial(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"EMISSION",            AIO_GL_MATERIAL_EMISSION},
    {"AMBIENT",             AIO_GL_MATERIAL_AMBIENT},
    {"DIFFUSE",             AIO_GL_MATERIAL_DIFFUSE},
    {"SPECULAR",            AIO_GL_MATERIAL_SPECULAR},
    {"AMBIENT_AND_DIFFUSE", AIO_GL_MATERIAL_AMBIENT_AND_DIFFUSE},
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumFog(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"LINEAR", AIO_GL_FOG_LINEAR},
    {"EXP",    AIO_GL_FOG_EXP},
    {"EXP2",   AIO_GL_FOG_EXP2}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumFogCoordSrc(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"FOG_COORDINATE", AIO_GL_FOG_COORD_SRC_FOG_COORDINATE},
    {"FRAGMENT_DEPTH", AIO_GL_FOG_COORD_SRC_FRAGMENT_DEPTH}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumFrontFace(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"CW",  AIO_GL_FRONT_FACE_CW},
    {"CCW", AIO_GL_FRONT_FACE_CCW}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumLightModelColorCtl(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"SINGLE_COLOR",
      AIO_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR},
    {"SEPARATE_SPECULAR_COLOR",
      AIO_GL_LIGHT_MODEL_COLOR_CONTROL_SEPARATE_SPECULAR_COLOR}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumLogicOp(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"CLEAR",         AIO_GL_LOGIC_OP_CLEAR},
    {"AND",           AIO_GL_LOGIC_OP_AND},
    {"AND_REVERSE",   AIO_GL_LOGIC_OP_AND_REVERSE},
    {"COPY",          AIO_GL_LOGIC_OP_COPY},
    {"AND_INVERTED",  AIO_GL_LOGIC_OP_AND_INVERTED},
    {"NOOP",          AIO_GL_LOGIC_OP_NOOP},
    {"XOR",           AIO_GL_LOGIC_OP_XOR},
    {"OR",            AIO_GL_LOGIC_OP_OR},
    {"NOR",           AIO_GL_LOGIC_OP_NOR},
    {"EQUIV",         AIO_GL_LOGIC_OP_EQUIV},
    {"INVERT",        AIO_GL_LOGIC_OP_INVERT},
    {"OR_REVERSE",    AIO_GL_LOGIC_OP_OR_REVERSE},
    {"COPY_INVERTED", AIO_GL_LOGIC_OP_COPY_INVERTED},
    {"NAND",          AIO_GL_LOGIC_OP_NAND},
    {"SET",           AIO_GL_LOGIC_OP_SET}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumPolyMode(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"POINT", AIO_GL_POLYGON_MODE_POINT},
    {"LINE",  AIO_GL_POLYGON_MODE_LINE},
    {"FILL",  AIO_GL_POLYGON_MODE_FILL},
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumShadeModel(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"FLAT",   AIO_GL_SHADE_MODEL_FLAT},
    {"SMOOTH", AIO_GL_SHADE_MODEL_SMOOTH}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumStencilOp(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"KEEP",      AIO_GL_STENCIL_OP_KEEP},
    {"ZERO",      AIO_GL_STENCIL_OP_ZERO},
    {"REPLACE",   AIO_GL_STENCIL_OP_REPLACE},
    {"INCR",      AIO_GL_STENCIL_OP_INCR},
    {"DECR",      AIO_GL_STENCIL_OP_DECR},
    {"INVERT",    AIO_GL_STENCIL_OP_INVERT},
    {"INCR_WRAP", AIO_GL_STENCIL_OP_INCR_WRAP},
    {"DECR_WRAP", AIO_GL_STENCIL_OP_DECR_WRAP}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumWrap(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"WRAP",        AIO_WRAP_MODE_WRAP},
    {"CLAMP",       AIO_WRAP_MODE_CLAMP},
    {"BORDER",      AIO_WRAP_MODE_BORDER},
    {"MIRROR",      AIO_WRAP_MODE_MIRROR},
    {"MIRROR_ONCE", AIO_WRAP_MODE_MIRROR_ONCE}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumMinfilter(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"NEAREST",     AIO_MINFILTER_NEAREST},
    {"LINEAR",      AIO_MINFILTER_LINEAR},
    {"ANISOTROPIC", AIO_MINFILTER_ANISOTROPIC}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumMipfilter(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"NONE",    AIO_MIPFILTER_NONE},
    {"NEAREST", AIO_MIPFILTER_NEAREST},
    {"LINEAR",  AIO_MIPFILTER_LINEAR}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumMagfilter(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"NEAREST", AIO_MAGFILTER_NEAREST},
    {"LINEAR",  AIO_MAGFILTER_LINEAR}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumShaderStage(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"VERTEX",      AIO_PIPELINE_STAGE_VERTEX},
    {"FRAGMENT",    AIO_PIPELINE_STAGE_FRAGMENT},
    {"TESSELATION", AIO_PIPELINE_STAGE_TESSELATION},
    {"GEOMETRY",    AIO_PIPELINE_STAGE_GEOMETRY}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumFace(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"POSITIVE_X", AIO_FACE_POSITIVE_X},
    {"NEGATIVE_X", AIO_FACE_NEGATIVE_X},
    {"POSITIVE_Y", AIO_FACE_POSITIVE_Y},
    {"NEGATIVE_Y", AIO_FACE_NEGATIVE_Y},
    {"POSITIVE_Z", AIO_FACE_POSITIVE_Z},
    {"NEGATIVE_Z", AIO_FACE_NEGATIVE_Z}
  };

  val = -1;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumDraw(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"GEOMETRY",         AIO_DRAW_GEOMETRY},
    {"SCENE_GEOMETRY",   AIO_DRAW_SCENE_GEOMETRY},
    {"SCENE_IMAGE",      AIO_DRAW_SCENE_IMAGE},
    {"FULL_SCREEN_QUAD", AIO_DRAW_FULL_SCREEN_QUAD},
    {"FULL_SCREEN_QUAD_PLUS_HALF_PIXEL",
      AIO_DRAW_FULL_SCREEN_QUAD_PLUS_HALF_PIXEL}
  };

  /* AIO_DRAW_READ_STR_VAL */
  val = 0;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumOpaque(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"A_ONE",    AIO_OPAQUE_A_ONE},
    {"RGB_ZERO", AIO_OPAQUE_RGB_ZERO},
    {"A_ZERO",   AIO_OPAQUE_A_ZERO},
    {"RGB_ONE",  AIO_OPAQUE_RGB_ONE}
  };

  val = 0;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumChannel(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"RGB",  AIO_FORMAT_CHANNEL_RGB},
    {"RGBA", AIO_FORMAT_CHANNEL_RGBA},
    {"RGBE", AIO_FORMAT_CHANNEL_RGBE},
    {"L",    AIO_FORMAT_CHANNEL_L},
    {"LA",   AIO_FORMAT_CHANNEL_LA},
    {"D",    AIO_FORMAT_CHANNEL_D}
  };

  val = 0;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumRange(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"SNORM", AIO_FORMAT_RANGE_SNORM},
    {"UNORM", AIO_FORMAT_RANGE_UNORM},
    {"SINT",  AIO_FORMAT_RANGE_SINT},
    {"UINT",  AIO_FORMAT_RANGE_UINT},
    {"FLOAT", AIO_FORMAT_RANGE_FLOAT}
  };

  val = 0;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

long _assetio_hide
aio_dae_fxEnumPrecision(const char * name) {
  long val;
  long glenums_len;
  long i;

  aio_dae_enum glenums[] = {
    {"DEFAULT", AIO_FORMAT_PRECISION_DEFAULT},
    {"LOW",     AIO_FORMAT_PRECISION_LOW},
    {"MID",     AIO_FORMAT_PRECISION_MID},
    {"HIGH",    AIO_FORMAT_PRECISION_HIGHT},
    {"MAX",     AIO_FORMAT_PRECISION_MAX}
  };

  val = 0;
  glenums_len = AIO_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strcasecmp(name, glenums[i].name) == 0) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}
