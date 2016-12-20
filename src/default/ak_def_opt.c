/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_def_opt.h"
#include <assert.h>

extern AkCoordSys   AK__Y_RH_VAL;
extern const char * AK_DEF_ID_PRFX;

uintptr_t AK_OPTIONS[] =
{
  false,                    /* 0: _INDICES_NONE                */
  true,                     /* 1: _INDICES_SINGLE_INTERLEAVED  */
  true,                     /* 2: _INDICES_SINGLE_SEPARATE     */
  true,                     /* 2: _INDICES_SINGLE              */
  (uintptr_t)&AK__Y_RH_VAL, /* 3: _COORD                       */
  (uintptr_t)&AK_DEF_ID_PRFX
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
