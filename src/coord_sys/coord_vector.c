/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../mem_common.h"
#include "coord_common.h"

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
