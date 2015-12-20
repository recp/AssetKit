/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_enums.h"
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
aio_dae_fxEnumFace(const char * name) {
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
