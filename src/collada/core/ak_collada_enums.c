/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_enums.h"
#include "../../ak_common.h"
#include <string.h>

AkEnum _assetkit_hide
ak_dae_enumInputSemantic(const char * name) {
  AkEnum val;
  long glenums_len;
  long i;

  ak_dae_enum glenums[] = {
    {"BINORMAL",        AK_INPUT_SEMANTIC_BINORMAL},
    {"COLOR",           AK_INPUT_SEMANTIC_COLOR},
    {"CONTINUITY",      AK_INPUT_SEMANTIC_CONTINUITY},
    {"IMAGE",           AK_INPUT_SEMANTIC_IMAGE},
    {"INPUT",           AK_INPUT_SEMANTIC_INPUT},
    {"IN_TANGENT",      AK_INPUT_SEMANTIC_IN_TANGENT},
    {"INTERPOLATION",   AK_INPUT_SEMANTIC_INTERPOLATION},
    {"INV_BIND_MATRIX", AK_INPUT_SEMANTIC_INV_BIND_MATRIX},
    {"JOINT",           AK_INPUT_SEMANTIC_JOINT},
    {"LINEAR_STEPS",    AK_INPUT_SEMANTIC_LINEAR_STEPS},
    {"MORPH_TARGET",    AK_INPUT_SEMANTIC_MORPH_TARGET},
    {"MORPH_WEIGHT",    AK_INPUT_SEMANTIC_MORPH_WEIGHT},
    {"NORMAL",          AK_INPUT_SEMANTIC_NORMAL},
    {"OUTPUT",          AK_INPUT_SEMANTIC_OUTPUT},
    {"OUT_TANGENT",     AK_INPUT_SEMANTIC_OUT_TANGENT},
    {"POSITION",        AK_INPUT_SEMANTIC_POSITION},
    {"TANGENT",         AK_INPUT_SEMANTIC_TANGENT},
    {"TEXBINORMAL",     AK_INPUT_SEMANTIC_TEXBINORMAL},
    {"TEXCOORD",        AK_INPUT_SEMANTIC_TEXCOORD},
    {"TEXTANGENT",      AK_INPUT_SEMANTIC_TEXTANGENT},
    {"UV",              AK_INPUT_SEMANTIC_UV},
    {"VERTEX",          AK_INPUT_SEMANTIC_VERTEX},
    {"WEIGHT",          AK_INPUT_SEMANTIC_WEIGHT},
  };

  /* COLLADA 1.5: ALWAYS is the default */
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
