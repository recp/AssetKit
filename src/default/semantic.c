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

#include "semantic.h"

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
ak_def_semantic(void) {
  return ak_def_sm_pairs;
}

uint32_t
ak_def_semanticc(void) {
  return AK_ARRAY_LEN(ak_def_sm_pairs);
}
