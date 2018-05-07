/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "def_semantic.h"

/* Prefix desc: P represents param, T represents type */

#define AK_PUSER  0 /* undefined by lib */
#define AK_PXYZ   1

#define AK_TFLOAT 0

AkTypeDesc ak_def_dtypes[] = {
  {"float", AKT_FLOAT, sizeof(float), 0}
};

AkInputSemanticPair ak_def_params[] = {
  {0, NULL, NULL},
  {3, &ak_def_dtypes[AK_TFLOAT], (char*[]){"X", "Y", "Z"}}
};

const AkInputSemanticPair* ak_def_sm_pairs[] = {
  /* _OTHER           */ &ak_def_params[AK_PUSER],
  /* _BINORMAL        */ &ak_def_params[AK_PUSER],
  /* _COLOR           */ &ak_def_params[AK_PUSER],
  /* _CONTINUITY      */ &ak_def_params[AK_PUSER],
  /* _IMAGE           */ &ak_def_params[AK_PUSER],
  /* _INPUT           */ &ak_def_params[AK_PUSER],
  /* _IN_TANGENT      */ &ak_def_params[AK_PUSER],
  /* _INTERPOLATION   */ &ak_def_params[AK_PUSER],
  /* _INV_BIND_MATRIX */ &ak_def_params[AK_PUSER],
  /* _JOINT           */ &ak_def_params[AK_PUSER],
  /* _LINEAR_STEPS    */ &ak_def_params[AK_PUSER],
  /* _MORPH_TARGET    */ &ak_def_params[AK_PUSER],
  /* _MORPH_WEIGHT    */ &ak_def_params[AK_PUSER],

  /* _NORMAL          */ &ak_def_params[AK_PXYZ],

  /* _OUTPUT          */ &ak_def_params[AK_PUSER],
  /* _OUT_TANGENT     */ &ak_def_params[AK_PUSER],

  /* _POSITION        */ &ak_def_params[AK_PXYZ],

  /* _TANGENT         */ &ak_def_params[AK_PUSER],
  /* _TEXBINORMAL     */ &ak_def_params[AK_PUSER],
  /* _TEXCOORD        */ &ak_def_params[AK_PUSER],
  /* _TEXTANGENT      */ &ak_def_params[AK_PUSER],
  /* _UV              */ &ak_def_params[AK_PUSER],
  /* _VERTEX          */ &ak_def_params[AK_PUSER],
  /* _WEIGHT          */ &ak_def_params[AK_PUSER]
};

const AkInputSemanticPair**
ak_def_semantic() {
  return ak_def_sm_pairs;
}

uint32_t
ak_def_semanticc() {
  return AK_ARRAY_LEN(ak_def_sm_pairs);
}
