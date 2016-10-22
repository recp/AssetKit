/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

AK_EXPORT
void
ak_coordCvtVectorTo(AkCoordSys *oldCoordSystem,
                    float      *oldVector,
                    AkCoordSys *newCoordSystem,
                    float      *newVector) {
  AkAxisAccessor a0, a1;

  a0.s_up    = AK_GET_SIGN(oldCoordSystem->up);
  a0.s_right = AK_GET_SIGN(oldCoordSystem->right);
  a0.s_fwd   = AK_GET_SIGN(oldCoordSystem->fwd);
  a0.up      = abs(oldCoordSystem->up)    - 1;
  a0.right   = abs(oldCoordSystem->right) - 1;
  a0.fwd     = abs(oldCoordSystem->fwd)   - 1;

  a1.s_up    = AK_GET_SIGN(newCoordSystem->up);
  a1.s_right = AK_GET_SIGN(newCoordSystem->right);
  a1.s_fwd   = AK_GET_SIGN(newCoordSystem->fwd);
  a1.up      = abs(newCoordSystem->up)    - 1;
  a1.right   = abs(newCoordSystem->right) - 1;
  a1.fwd     = abs(newCoordSystem->fwd)   - 1;

  newVector[a1.up]    = oldVector[a0.up]    * a0.s_up    * a1.s_up;
  newVector[a1.right] = oldVector[a0.right] * a0.s_right * a1.s_right;
  newVector[a1.fwd]   = oldVector[a0.fwd]   * a0.s_fwd   * a1.s_fwd;
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
  size_t i;
  float  tmp[3];

  for (i = 0; i < len; i += 3) {
    ak_coordCvtVectorTo(oldCoordSystem,
                        &vectorArray[i],
                        newCoordSystem,
                        tmp);
    vectorArray[i]     = tmp[0];
    vectorArray[i + 1] = tmp[1];
    vectorArray[i + 2] = tmp[2];
  }
}
