/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../memory_common.h"
#include "coord_common.h"
#include <cglm/cglm.h>
#include <float.h>

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
  glm_vec3_scale(angles, rotDirection, angles);

  /* apply new rotation */
  glm_euler_by_order(angles,
                     glm_euler_order(eulerNew),
                     rot);

  /* find new scaling factors */
  AK_CVT_VEC_NOSIGN(scalev);

  /* apply new scaling factors */
  glm_mat4_copy(GLM_MAT4_IDENTITY, scale);
  scale[0][0] = scalev[0];
  scale[1][1] = scalev[1];
  scale[2][2] = scalev[2];

  glm_vec4_copy(transform[3], pos);
  glm_mul(rot, scale, transform);

  /* apply new translation */
  AK_CVT_VEC(pos)

  glm_vec4_copy(pos, transform[3]);
}

AK_EXPORT
void
ak_coordFindTransform(AkTransform *transform,
                      AkCoordSys  *oldCoordSys,
                      AkCoordSys  *newCoordSys) {
  AkHeap        *heap;
  AkObject      *firstTrans, *lastTrans;
  AkAxisAccessor a0, a1;
  ivec3          oriOld, oriNew;
  vec3           x1, y1, z1, oth, axis;
  float          angle;

  if (oldCoordSys == newCoordSys
      || ak_coordOrientationIsEq(oldCoordSys, newCoordSys))
    return;

  firstTrans = lastTrans = NULL;

  heap = ak_heap_getheap(transform);
  if (!transform->base)
    ak_transformFreeBase(transform);

  ak_coordAxisAccessors(oldCoordSys, newCoordSys, &a0, &a1);

  ak_coordToiVec3(oldCoordSys, oriOld);
  ak_coordToiVec3(newCoordSys, oriNew);

  glm_vec3_broadcast(0.0f, x1);
  glm_vec3_broadcast(0.0f, y1);
  glm_vec3_broadcast(0.0f, z1);
  x1[abs(oriOld[0]) - 1] = (float)glm_sign(oriOld[0]);
  y1[abs(oriOld[1]) - 1] = (float)glm_sign(oriOld[1]);
  z1[abs(oriOld[2]) - 1] = (float)glm_sign(oriOld[2]);

  /* step-1: X axis; a rotation will be enough */

  /* find rotation axis */
  glm_vec3_broadcast(0.0f, oth);
  oth[abs(oriNew[0]) - 1] = (float)glm_sign(oriNew[0]);
  glm_vec3_cross(x1, oth, axis);

  /* angle for x axis */
  angle = glm_vec3_angle(x1, oth);

  /* append transform */
  if (angle != 0.0f) {
    AkObject *rotObj;
    AkRotate *rot;

    rotObj = ak_objAlloc(heap,
                         transform,
                         sizeof(*rotObj),
                         AKT_ROTATE,
                         true);

    rot = ak_objGet(rotObj);
    glm_vec3_copy(axis, rot->val);
    rot->val[3] = angle;


    /* rotate y and z */
    glm_vec3_rotate(y1, angle, axis);
    glm_vec3_rotate(z1, angle, axis);

    firstTrans = lastTrans = rotObj;
  }

  /* step-2: Y axis; an extra rotation around X will be enough */

  /* find rotation axis */
  glm_vec3_broadcast(0.0f, oth);
  oth[abs(oriNew[1]) - 1] = (float)glm_sign(oriNew[1]);
  glm_vec3_cross(y1, oth, axis);

  /* angle for y axis */
  angle = glm_vec3_angle(y1, oth);

  /* append transform */
  if (angle != 0.0f) {
    AkObject *rotObj;
    AkRotate *rot;

    rotObj = ak_objAlloc(heap,
                         transform,
                         sizeof(*rotObj),
                         AKT_ROTATE,
                         true);

    rot = ak_objGet(rotObj);
    glm_vec3_copy(axis, rot->val);
    rot->val[3] = angle;

    /* rotate z, we know that x will keep it orientation */
    glm_vec3_rotate(z1, angle, axis);

    if (lastTrans)
      lastTrans->next = rotObj;
    else
      firstTrans = rotObj;
    lastTrans = rotObj;
  }

  /* step-3: Z axis; we can't add rotation, we need negative scale */

  /* check old and new Z axes are same */
  glm_vec3_broadcast(0.0f, oth);
  oth[abs(oriNew[2]) - 1] = (float)glm_sign(oriNew[2]);
  if (!glm_vec3_eqv_eps(z1, oth)) {
    /* fix Z by negative scale */
    AkObject *scaleObj;
    AkScale  *scale;

    scaleObj = ak_objAlloc(heap,
                           transform,
                           sizeof(*scaleObj),
                           AKT_SCALE,
                           true);

    scale = ak_objGet(scaleObj);
    scale->val[0] =  1.0f;
    scale->val[1] =  1.0f;
    scale->val[2] = -1.0f;

    if (lastTrans)
      lastTrans->next = scaleObj;
    else
      firstTrans = scaleObj;
  }

  transform->base = firstTrans;
}
