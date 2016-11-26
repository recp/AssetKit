/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_def_opt.h"

extern AkCoordSys AK__Y_RH_VAL;

uintptr_t AK_OPTIONS[] =
{
  false,                   /* 0: _SOURCE_FIX_NONE         */
  true,                    /* 1: _SOURCE_FIX_INTERLEAVED  */
  true,                    /* 2: _SOURCE_FIX_INDIVIDUAL   */
  (uintptr_t)&AK__Y_RH_VAL /* 3: _COORD                   */
};

AK_EXPORT
void
ak_opt_set(AkOption option, uintptr_t value) {
  if ((uint32_t)option >= AK_ARRAY_LEN(AK_OPTIONS))
    return;

  AK_OPTIONS[option] = value;
}

AK_EXPORT
uintptr_t
ak_opt_get(AkOption option) {
  if ((uint32_t)option >= AK_ARRAY_LEN(AK_OPTIONS))
    return 0;

  return AK_OPTIONS[option];
}
