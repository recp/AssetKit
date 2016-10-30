/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_coord_common_h
#define ak_coord_common_h

#include "../ak_common.h"
#include "../ak_memory_common.h"

#define AK_CVT_VEC_TO(X0, X1)                                                 \
  X1[a1.up]    = X0[a0.up]    * a0.s_up    * a1.s_up;                         \
  X1[a1.right] = X0[a0.right] * a0.s_right * a1.s_right;                      \
  X1[a1.fwd]   = X0[a0.fwd]   * a0.s_fwd   * a1.s_fwd;

#define AK_CVT_VEC_NOSIGN_TO(X0, X1)                                          \
  X1[a1.up]    = X0[a0.up];                                                   \
  X1[a1.right] = X0[a0.right];                                                \
  X1[a1.fwd]   = X0[a0.fwd];

#define AK_CVT_VEC(X)                                                         \
  AK_CVT_VEC_TO(X, tmp)                                                       \
  X[0] = tmp[0];                                                              \
  X[1] = tmp[1];                                                              \
  X[2] = tmp[2];

#define AK_CVT_VEC_NOSIGN(X)                                                  \
  AK_CVT_VEC_NOSIGN_TO(X, tmp)                                                \
  X[0] = tmp[0];                                                              \
  X[1] = tmp[1];                                                              \
  X[2] = tmp[2];

AK_INLINE
void
ak_coordAxisAccessors(AkCoordSys * __restrict oldCoordSys,
                      AkCoordSys * __restrict newCoordSys,
                      AkAxisAccessor * __restrict a0,
                      AkAxisAccessor * __restrict a1) {
  a0->s_up    = AK_GET_SIGN(oldCoordSys->up);
  a0->s_right = AK_GET_SIGN(oldCoordSys->right);
  a0->s_fwd   = AK_GET_SIGN(oldCoordSys->fwd);
  a0->up      = abs(oldCoordSys->up)    - 1;
  a0->right   = abs(oldCoordSys->right) - 1;
  a0->fwd     = abs(oldCoordSys->fwd)   - 1;

  a1->s_up    = AK_GET_SIGN(newCoordSys->up);
  a1->s_right = AK_GET_SIGN(newCoordSys->right);
  a1->s_fwd   = AK_GET_SIGN(newCoordSys->fwd);
  a1->up      = abs(newCoordSys->up)    - 1;
  a1->right   = abs(newCoordSys->right) - 1;
  a1->fwd     = abs(newCoordSys->fwd)   - 1;
}

#endif /* ak_coord_common_h */
