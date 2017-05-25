/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_def_light.h"

AkDirectionalLight akdef_light_tcommon = {
  .direction = {0.0f, 0.0f, -1.0f},
  .type      = AK_LIGHT_TYPE_DIRECTIONAL,
  .color     = { {1.0, 1.0, 1.0, 1.0} },
  .ctype     = 0
};

AkLight akdef_light = {
  .name      = "default",
  .tcommon   = &akdef_light_tcommon,
  .technique = NULL,
  .extra     = NULL,
  .next      = NULL
};

const AkLight*
ak_def_light() {
  return &akdef_light;
}
