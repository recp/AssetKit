/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_def_opt.h"
#include <string.h>
#include <assert.h>

extern AkCoordSys   AK__Y_RH_VAL;
extern const char * AK_DEF_ID_PRFX;

const char *ak__def_techniques[] = {"common", NULL};

uintptr_t AK_OPTIONS[] =
{
  false,                           /* 0:  _INDICES_NONE                */
  false,                           /* 1:  _INDICES_SINGLE_INTERLEAVED  */
  false,                           /* 2:  _INDICES_SINGLE_SEPARATE     */
  false,                           /* 3:  _INDICES_SINGLE              */
  true,                            /* 4:  _NO_INDICES_INTERLEAVED      */
  false,                           /* 5:  _NO_INDICES_SEPARATE         */
  (uintptr_t)&AK__Y_RH_VAL,        /* 6:  _COORD                       */
  (uintptr_t)&AK_DEF_ID_PRFX,      /* 7:  _DEFAULT_ID_PREFIX           */
  false,                           /* 8:  _COMPUTE_BBOX                */
  true,                            /* 9: _TRIANGULATE                 */
  true,                            /* 10: _GEN_NORMALS_IF_NEEDED       */
  AK_PROFILE_TYPE_COMMON,          /* 11: _DEFAULT_PROFILE             */
  true,                            /* 12: _EFFECT_PROFILE              */
  (uintptr_t)ak__def_techniques,   /* 13: _TECHNIQUE                   */
  (uintptr_t)ak__def_techniques,   /* 14: _TECHNIQUE_FX                */
  false,                           /* 15: _ZERO_INDEXED_INPUT          */
  true,                            /* 16: _IMAGE_LOAD_FLIP_VERTICALLY  */
  true,                            /* 17: _ADD_DEFAULT_CAMERA          */
  false,                           /* 18: _ADD_DEFAULT_LIGHT           */
  AK_COORD_CVT_DEFAULT,            /* 19: _COORD_CONVERT_TYPE          */
  true,                            /* 20: _BUGFIXES                    */
  true                             /* 21: _GLTF_EXT_SPEC_GLOSS         */
};

AK_EXPORT
void
ak_opt_set(AkOption option, uintptr_t value) {
  assert((uint32_t)option < AK_ARRAY_LEN(AK_OPTIONS));

  AK_OPTIONS[option] = value;
}

AK_EXPORT
uintptr_t
ak_opt_get(AkOption option) {
  assert((uint32_t)option < AK_ARRAY_LEN(AK_OPTIONS));

  return AK_OPTIONS[option];
}

AK_EXPORT
void
ak_opt_set_str(AkOption option, const char *value) {
  assert((uint32_t)option < AK_ARRAY_LEN(AK_OPTIONS));

  AK_OPTIONS[option] = (uintptr_t)ak_strdup(NULL, value);
}
