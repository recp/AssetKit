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
ak_coordCvtSequenceTo(AkCoordSys *oldCoordSystem,
                      float      *oldSequence,
                      AkCoordSys *newCoordSystem,
                      float      *newSequence) {
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

  newSequence[a1.up]    = oldSequence[a0.up]    * a0.s_up    * a1.s_up;
  newSequence[a1.right] = oldSequence[a0.right] * a0.s_right * a1.s_right;
  newSequence[a1.fwd]   = oldSequence[a0.fwd]   * a0.s_fwd   * a1.s_fwd;
}

AK_EXPORT
void
ak_coordCvtSequence(AkCoordSys *oldCoordSystem,
                    float      *sequence,
                    AkCoordSys *newCoordSystem) {
  float tmp[3];
  ak_coordCvtSequenceTo(oldCoordSystem,
                        sequence,
                        newCoordSystem,
                        tmp);

  sequence[0] = tmp[0];
  sequence[1] = tmp[1];
  sequence[2] = tmp[2];
}

AK_EXPORT
void
ak_coordCvtSequenceN(AkCoordSys *oldCoordSystem,
                     float      *sequenceArray,
                     size_t     len,
                     AkCoordSys *newCoordSystem) {
  size_t i;
  float  tmp[3];

  for (i = 0; i < len; i += 3) {
    ak_coordCvtSequenceTo(oldCoordSystem,
                          &sequenceArray[i],
                          newCoordSystem,
                          tmp);
    sequenceArray[i]     = tmp[0];
    sequenceArray[i + 1] = tmp[1];
    sequenceArray[i + 2] = tmp[2];
  }
}
