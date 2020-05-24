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

AK_EXPORT
void
ak_coordCvtVectorTo(AkCoordSys *oldCoordSystem,
                    float      *oldVector,
                    AkCoordSys *newCoordSystem,
                    float      *newVector) {
  AkAxisAccessor a0, a1;

  ak_coordAxisAccessors(oldCoordSystem, newCoordSystem, &a0, &a1);
  AK_CVT_VEC_TO(oldVector, newVector)
}

AK_EXPORT
void
ak_coordCvtVector(AkCoordSys *oldCoordSystem,
                  float      *vector,
                  AkCoordSys *newCoordSystem) {
  float tmp[3];
  ak_coordCvtVectorTo(oldCoordSystem,
                      vector,
                      newCoordSystem,
                      tmp);

  vector[0] = tmp[0];
  vector[1] = tmp[1];
  vector[2] = tmp[2];
}

AK_EXPORT
void
ak_coordCvtVectors(AkCoordSys *oldCoordSystem,
                   float      *vectorArray,
                   size_t      len,
                   AkCoordSys *newCoordSystem) {
  AkAxisAccessor a0, a1;

  size_t i;
  float  tmp[3];

  ak_coordAxisAccessors(oldCoordSystem, newCoordSystem, &a0, &a1);

  for (i = 0; i < len; i += 3) {
    AK_CVT_VEC_TO((vectorArray + i), tmp)
    vectorArray[i]     = tmp[0];
    vectorArray[i + 1] = tmp[1];
    vectorArray[i + 2] = tmp[2];
  }
}
