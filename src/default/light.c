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

#include "light.h"

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
ak_def_light(void) {
  return &akdef_light;
}
