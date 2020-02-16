/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "enum.h"
#include "../../common.h"
#include "../../xml.h"
#include <string.h>

AkEnum _assetkit_hide
dae_wrap(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

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

AkEnum _assetkit_hide
dae_minfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

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

AkEnum _assetkit_hide
dae_mipfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

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

AkEnum _assetkit_hide
dae_magfilter(const xml_t * __restrict xml) {
  AkEnum val;
  long   glenums_len, i;

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

AkEnum _assetkit_hide
dae_face(const xml_attr_t * __restrict xatt) {
  AkEnum val;
  long   glenums_len, i;

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
    if (strncasecmp(xatt->val, glenums[i].name, xatt->valsize)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum _assetkit_hide
dae_opaque(const xml_attr_t * __restrict xatt) {
  AkEnum val;
  long  glenums_len, i;

  dae_enum glenums[] = {
    {"A_ONE",    AK_OPAQUE_A_ONE},
    {"RGB_ZERO", AK_OPAQUE_RGB_ZERO},
    {"A_ZERO",   AK_OPAQUE_A_ZERO},
    {"RGB_ONE",  AK_OPAQUE_RGB_ONE}
  };

  val         = AK_OPAQUE_A_ONE;
  glenums_len = AK_ARRAY_LEN(glenums);

  for (i = 0; i < glenums_len; i++) {
    if (strncasecmp(xatt->val, glenums[i].name, xatt->valsize)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum _assetkit_hide
dae_enumChannel(const char *name, size_t len) {
  AkEnum val;
  long   glenums_len, i;

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
    if (strncasecmp(name, glenums[i].name, len)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum _assetkit_hide
dae_range(const char *name, size_t len) {
  AkEnum val;
  long   glenums_len, i;

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
    if (strncasecmp(name, glenums[i].name, len)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}

AkEnum _assetkit_hide
dae_precision(const char *name, size_t len) {
  AkEnum val;
  long   glenums_len, i;

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
    if (strncasecmp(name, glenums[i].name, len)) {
      val = glenums[i].val;
      break;
    }
  }

  return val;
}
