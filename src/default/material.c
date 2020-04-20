/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "material.h"
#include "../common.h"

float ak__def_transpval = 1.0f;
AkFloatOrParam ak__def_transparency = {
  .val   = &ak__def_transpval,
  .param = NULL
};

AkFloatOrParam*
ak_def_transparency(void) {
  return &ak__def_transparency;
}
