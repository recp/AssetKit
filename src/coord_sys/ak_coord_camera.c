/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "ak_coord_common.h"
#include <cglm.h>

AK_EXPORT
void
ak_coordFixCamOri(AkCoordSys *oldCoordSys,
                  AkCoordSys *newCoordSys,
                  AkFloat4x4  transform) {
  ivec3          camOri, camOriNew;
  AkAxisAccessor a0, a1;
  vec3           v1, v2, v3;
  float          angle;

  if (ak_coordOrientationIsEq(oldCoordSys, newCoordSys))
    return;

  ak_coordAxisAccessors(oldCoordSys, newCoordSys, &a0,  &a1);
  ak_coordAxisToiVec3(oldCoordSys->cameraOrientation, camOri);

  AK_CVT_VEC_TO(camOri, camOriNew)

  glm_vec_broadcast(0.0f, v1);
  glm_vec_broadcast(0.0f, v2);

  v1[abs(camOri[2]) - 1] = glm_sign(camOri[2]);
  v2[abs(camOriNew[2]) - 1] = glm_sign(camOriNew[2]);
  glm_vec_cross(v1, v2, v3);

  angle = glm_vec_angle(v1, v2);

  if (angle != 0.0f) {
    /* apply rotation for forward direction */
    glm_rotate(transform, angle, v3);

    /* now forward1 == forward0 == v1, fix up vector */
    /* v2 is new up */
    glm_vec_cross(v1, v3, v2);
  } else {
    /* forward1 == forward0 [already], Up = Right x Fwd */
    glm_vec_broadcast(0.0f, v3);
    v3[abs(camOri[0]) - 1] = glm_sign(camOri[0]);
    glm_vec_cross(v3, v1, v2);
  }

  glm_vec_broadcast(0.0f, v1);
  v1[abs(camOri[1]) - 1] = glm_sign(camOri[1]);

  angle = glm_vec_angle(v1, v2);

  /* apply rotation for up direction */
  if (angle != 0.0f)
    glm_rotate(transform, angle, v2);
}
