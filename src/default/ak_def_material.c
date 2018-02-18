/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_def_material.h"
#include "../ak_common.h"

float ak__def_transpval = 1.0f;
AkFxFloatOrParam ak__def_transparency = {
  .val   = &ak__def_transpval,
  .param = NULL
};

AkFxFloatOrParam*
ak_def_transparency(void) {
  return &ak__def_transparency;
}
