/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "common.h"
#include <cglm/cglm.h>

_assetkit_hide
void
ak_coordRotForFixedCoord(AkCoordSys        *oldCoordSys,
                         AkCoordSys        *newCoordSys,
                         AkAxisOrientation *oldAxes,
                         AkAxisOrientation *newAxes,
                         vec4               fwdAxis,
                         vec4               upAxis);

_assetkit_hide
void
ak_coordRotForFixedCoord(AkCoordSys        *oldCoordSys,
                         AkCoordSys        *newCoordSys,
                         AkAxisOrientation *oldAxes,
                         AkAxisOrientation *newAxes,
                         vec4               fwdAxis,
                         vec4               upAxis) {
  ivec3          camOriOld, camOriNew;
  vec3           v1, v2, v3, tmp;
  AkAxisAccessor a0, a1;

  ak_coordAxisCamAccessors(newCoordSys,
                           &a0,
                           &a1);

  ak_coordAxisOri(oldCoordSys, *oldAxes, camOriOld);
  ak_coordAxisOri(newCoordSys, *newAxes, camOriNew);

  glm_vec3_broadcast(0.0f, v1);
  glm_vec3_broadcast(0.0f, v2);

  /* we want to rotate from new to old!!! */
  v1[abs(camOriNew[2]) - 1] = (float)glm_sign(camOriNew[2]);
  v2[abs(camOriOld[2]) - 1] = (float)glm_sign(camOriOld[2]);
  glm_vec3_cross(v1, v2, v3);

  /* angle for forward axis */
  fwdAxis[3] = glm_vec3_angle(v1, v2);
  if (fwdAxis[3] != 0.0f) {
    /* forward axis */
    glm_vec3_copy(v3, fwdAxis);

    /* convert to current coord sys */
    AK_CVT_VEC(fwdAxis)
  }

  /* up axis */
  glm_vec3_broadcast(0.0f, v1);
  glm_vec3_broadcast(0.0f, v2);

  v1[abs(camOriNew[1]) - 1] = (float)glm_sign(camOriNew[1]);
  v2[abs(camOriOld[1]) - 1] = (float)glm_sign(camOriOld[1]);

  /* rotate with fwd to find new up (rotated) */
  glm_vec3_rotate(v1, fwdAxis[3], v3);

  glm_vec3_cross(v1, v2, v3);

  /* angle for up axis */
  upAxis[3] = glm_vec3_angle(v1, v2);

  /* up direction */
  if (upAxis[3] != 0.0f) {
    /* up axis */
    glm_vec3_copy(v3, upAxis);

    /* convert to current coord sys */
    AK_CVT_VEC(upAxis)

    /* rotate found axis with fwd */
    if (fwdAxis[3] != 0.0f)
      glm_vec3_rotate(upAxis, -fwdAxis[3], fwdAxis);
  }
}

AK_EXPORT
void
ak_coordFixCamOri(AkCoordSys *oldCoordSys,
                  AkCoordSys *newCoordSys,
                  AkFloat4x4  transform) {
  vec4 fwdAxis, upAxis;

  if (ak_coordOrientationIsEq(oldCoordSys, newCoordSys))
    return;

  memset(fwdAxis, 0, sizeof(fwdAxis));
  memset(upAxis,  0, sizeof(upAxis));

  ak_coordRotForFixedCoord(oldCoordSys,
                           newCoordSys,
                           &oldCoordSys->cameraOrientation,
                           &newCoordSys->cameraOrientation,
                           fwdAxis,
                           upAxis);

  /* apply rotation for forward direction */
  if (fwdAxis[3] != 0.0f)
    glm_rotate(transform, fwdAxis[3], fwdAxis);

  /* apply rotation for up direction */
  if (upAxis[3] != 0.0f)
    glm_rotate(transform, upAxis[3], upAxis);
}

AK_EXPORT
void
ak_coordRotNodeForFixedCoord(AkDoc     *doc,
                             void      *memparent,
                             AkObject **destTransform) {
  AkHeap     *heap;
  AkCoordSys *oldCoordSys,  *newCoordSys;
  AkObject   *transformFwd, *transformUp;
  AkRotate   *rotate;
  vec4        fwdAxis, upAxis;

  oldCoordSys = doc->coordSys;
  newCoordSys = (void *)ak_opt_get(AK_OPT_COORD);

  *destTransform = NULL;
  if (ak_coordOrientationIsEq(oldCoordSys, newCoordSys))
    return;

  memset(fwdAxis, 0, sizeof(fwdAxis));
  memset(upAxis,  0, sizeof(upAxis));

  ak_coordRotForFixedCoord(oldCoordSys,
                           newCoordSys,
                           &oldCoordSys->cameraOrientation,
                           &newCoordSys->cameraOrientation,
                           fwdAxis,
                           upAxis);

  heap = ak_heap_getheap(doc);

  /* we assume that we only need two rotation for fwd and up (not right) */

  /* rotation for forward direction */
  if (fwdAxis[3] != 0.0f) {
    transformFwd = ak_objAlloc(heap,
                               memparent,
                               sizeof(*rotate),
                               AKT_ROTATE,
                               true);

    rotate = ak_objGet(transformFwd);
    glm_vec4_copy(fwdAxis, rotate->val);

    *destTransform = transformFwd;
  }

  /* rotation for up direction */
  if (upAxis[3] != 0.0f) {
    transformUp = ak_objAlloc(heap,
                              memparent,
                              sizeof(*rotate),
                              AKT_ROTATE,
                              true);

    rotate = ak_objGet(transformUp);
    glm_vec4_copy(upAxis, rotate->val);

    if (*destTransform)
      (*destTransform)->next = transformUp;
    else
      *destTransform = transformUp;
  }
}
