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


#include "../common.h"
#include "common.h"
#include <cglm/cglm.h>

void
ak_coordAxisOri(AkCoordSys * __restrict coordSys,
                AkAxisOrientation       axis,
                int                     ori[3]) {
  int axisOri[3];
  int coord[3];
  int i, j;

  ak_coordAxisToiVec3(axis, axisOri);
  ak_coordToiVec3(coordSys, coord);

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++)
      if (axisOri[i] == coord[j])
        ori[i] = (j + 1) * glm_sign(axisOri[j]);
      else if (abs(axisOri[i]) == abs(coord[j]))
        ori[i] = -(j + 1) * glm_sign(axisOri[j]);
  }
}

void
ak_coordAxisOriAbs(AkCoordSys * __restrict coordSys,
                   AkAxisOrientation       axis,
                   int                     newAxisOri[3]) {
  ak_coordAxisOri(coordSys, axis, newAxisOri);

  newAxisOri[0] = abs(newAxisOri[0]) - 1;
  newAxisOri[1] = abs(newAxisOri[1]) - 1;
  newAxisOri[2] = abs(newAxisOri[2]) - 1;
}
