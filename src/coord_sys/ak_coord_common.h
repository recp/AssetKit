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
  AkAxisOrientation oldAxis, newAxis;

  oldAxis = oldCoordSys->axis;
  newAxis = newCoordSys->axis;

  a0->s_up    = AK_GET_SIGN(oldAxis.up);
  a0->s_right = AK_GET_SIGN(oldAxis.right);
  a0->s_fwd   = AK_GET_SIGN(oldAxis.fwd);
  a0->up      = abs(oldAxis.up)    - 1;
  a0->right   = abs(oldAxis.right) - 1;
  a0->fwd     = abs(oldAxis.fwd)   - 1;

  a1->s_up    = AK_GET_SIGN(newAxis.up);
  a1->s_right = AK_GET_SIGN(newAxis.right);
  a1->s_fwd   = AK_GET_SIGN(newAxis.fwd);
  a1->up      = abs(newAxis.up)    - 1;
  a1->right   = abs(newAxis.right) - 1;
  a1->fwd     = abs(newAxis.fwd)   - 1;
}

AK_INLINE
void
ak_coordAxisCamAccessors(AkCoordSys * __restrict newCoordSys,
                         AkAxisAccessor * __restrict a0,
                         AkAxisAccessor * __restrict a1) {
  AkAxisOrientation oldAxis, newAxis;

  oldAxis = newCoordSys->cameraOrientation;
  newAxis = newCoordSys->axis;

  a0->s_up    = AK_GET_SIGN(oldAxis.up);
  a0->s_right = AK_GET_SIGN(oldAxis.right);
  a0->s_fwd   = AK_GET_SIGN(oldAxis.fwd);
  a0->up      = abs(oldAxis.up)    - 1;
  a0->right   = abs(oldAxis.right) - 1;
  a0->fwd     = abs(oldAxis.fwd)   - 1;

  a1->s_up    = AK_GET_SIGN(newAxis.up);
  a1->s_right = AK_GET_SIGN(newAxis.right);
  a1->s_fwd   = AK_GET_SIGN(newAxis.fwd);
  a1->up      = abs(newAxis.up)    - 1;
  a1->right   = abs(newAxis.right) - 1;
  a1->fwd     = abs(newAxis.fwd)   - 1;
}

#endif /* ak_coord_common_h */
