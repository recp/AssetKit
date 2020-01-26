/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "enum.h"
#include "../../common.h"
#include <string.h>

AkEnum _assetkit_hide
dae_fxEnumMaterial(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumWrap(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumMinfilter(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumMipfilter(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumMagfilter(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumFace(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumOpaque(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumChannel(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

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
dae_fxEnumRange(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
dae_fxEnumPrecision(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  dae_enum glenums[] = {
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
