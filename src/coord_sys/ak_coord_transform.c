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
ak_coordCvtTransform(AkCoordSys *oldCoordSystem,
                     AkFloat4x4  transform,
                     AkCoordSys *newCoordSystem) {
  mat4               rot, scale;
  vec4               pos;
  vec3               scalev, angles, tmp;
  AkAxisAccessor     a0, a1;
  ivec3              eulerNew;
  AkAxisRotDirection rotDirection;

  rotDirection = (oldCoordSystem->rotDirection + 1)
                      * (newCoordSystem->rotDirection + 1);

  ak_coordAxisAccessors(oldCoordSystem, newCoordSystem, &a0, &a1);

  /* decompose rotation and scaling factors */
  glm_decompose_rs(transform, rot, scalev);

  /* extract euler angles XYZ */
  glm_euler_angles(rot, angles);
  AK_CVT_VEC(angles);

  /* find new euler sequence */
  ak_coordAxisOriAbs(newCoordSystem,
                     oldCoordSystem->axis,
                     eulerNew);

  /* apply new rotation direction */
  glm_vec_scale(angles, rotDirection, angles);

  /* apply new rotation */
  glm_euler_by_order(angles,
                     glm_euler_order(eulerNew),
                     rot);

  /* find new scaling factors */
  AK_CVT_VEC_NOSIGN(scalev);

  /* apply new scaling factors */
  glm_mat4_dup(GLM_MAT4_IDENTITY, scale);
  scale[0][0] = scalev[0];
  scale[1][1] = scalev[1];
  scale[2][2] = scalev[2];

  glm_vec4_dup(transform[3], pos);
  glm_mul(rot, scale, transform);

  /* apply new translation */
  AK_CVT_VEC(pos)

  glm_vec4_dup(pos, transform[3]);
}
